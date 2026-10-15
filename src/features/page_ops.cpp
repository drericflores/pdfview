#include "features/page_ops.h"

#include <functional>
#include <string>

#include "core/document_model.h"
#include "core/pdf_ops.h"
#include "core/util.h"
#include "ui/dialogs.h"
#include "ui/menu_builder.h"

namespace efpdf {
namespace {

using Op = std::function<bool(const std::string &in, const std::string &out,
                              const std::string &password, std::string &err)>;

bool run(AppContext &ctx, const Op &op, int new_page) {
    DocumentModel &model = ctx.model;
    if (!model.is_open()) return false;

    std::string in, err;
    if (!model.snapshot(in, err)) {
        ui::show_error(ctx.window, "Could not prepare the document: " + err);
        return false;
    }
    const std::string out = util::make_temp_path();
    if (out.empty()) {
        util::remove_file(in);
        ui::show_error(ctx.window, "Could not create a temporary file.");
        return false;
    }

    const bool ok = op(in, out, model.password(), err);
    util::remove_file(in);
    if (!ok) {
        util::remove_file(out);
        ui::show_error(ctx.window, "The operation failed: " + err);
        return false;
    }
    if (!model.replace_with(out, new_page, err)) {
        util::remove_file(out);
        ui::show_error(ctx.window, "Could not load the result: " + err);
        return false;
    }
    return true;
}

}  // namespace

PageOps::PageOps(AppContext &ctx) : ctx_(ctx) {
    const Gdk::ModifierType ctrl = Gdk::CONTROL_MASK;
    const Gdk::ModifierType ctrl_alt = Gdk::CONTROL_MASK | Gdk::MOD1_MASK;
    auto &menus = ctx.menus;

    menus.add_item("_Edit", "Rotate Page _Left", [this] { rotate(-90); }, GDK_KEY_l, ctrl);
    menus.add_item("_Edit", "Rotate Page _Right", [this] { rotate(90); }, GDK_KEY_r, ctrl);
    menus.add_item("_Edit", "_Delete Page", [this] { delete_current(); });
    menus.add_item("_Edit", "Move Page _Up", [this] { move_current(-1); }, GDK_KEY_Up,
                   ctrl_alt);
    menus.add_item("_Edit", "Move Page Do_wn", [this] { move_current(1); }, GDK_KEY_Down,
                   ctrl_alt);
    menus.add_separator("_Edit");
}

void PageOps::rotate(int degrees) {
    const int page = ctx_.model.current_page();
    run(ctx_,
        [page, degrees](const std::string &in, const std::string &out,
                        const std::string &pw, std::string &err) {
            return pdf_ops::rotate_page(in, out, page, degrees, pw, err);
        },
        page);
}

void PageOps::delete_current() {
    DocumentModel &model = ctx_.model;
    if (!model.is_open()) return;
    if (model.page_count() < 2) {
        ui::show_error(ctx_.window, "A PDF must keep at least one page.");
        return;
    }
    const int page = model.current_page();
    if (!ui::ask_yes_no(ctx_.window, "Delete page " + std::to_string(page + 1) + "?")) {
        return;
    }
    run(ctx_,
        [page](const std::string &in, const std::string &out, const std::string &pw,
               std::string &err) { return pdf_ops::delete_page(in, out, page, pw, err); },
        page);
}

bool PageOps::move_page(int from, int to) {
    DocumentModel &model = ctx_.model;
    if (!model.is_open() || from == to) return false;
    const int count = model.page_count();
    if (from < 0 || to < 0 || from >= count || to >= count) return false;
    return run(ctx_,
               [from, to](const std::string &in, const std::string &out,
                          const std::string &pw, std::string &err) {
                   return pdf_ops::move_page(in, out, from, to, pw, err);
               },
               to);
}

void PageOps::move_current(int delta) {
    const int from = ctx_.model.current_page();
    move_page(from, from + delta);
}

}  // namespace efpdf
