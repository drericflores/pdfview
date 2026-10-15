#include "core/document_model.h"

#include <glib/gstdio.h>

#include <algorithm>

#include "core/util.h"

namespace efpdf {

DocumentModel::~DocumentModel() { discard_work_file(); }

int DocumentModel::page_count() const {
    return doc_ ? poppler_document_get_n_pages(doc_.get()) : 0;
}

PagePtr DocumentModel::page(int index) const {
    if (!doc_ || index < 0 || index >= page_count()) return PagePtr();
    return PagePtr(poppler_document_get_page(doc_.get(), index));
}

LoadResult DocumentModel::load_file(const std::string &file,
                                    const std::string &password, DocumentPtr &out,
                                    std::string &err) const {
    const std::string uri = util::to_uri(file);
    if (uri.empty()) {
        err = "Could not resolve the file path.";
        return LoadResult::Failed;
    }

    GError *error = nullptr;
    PopplerDocument *d = poppler_document_new_from_file(
        uri.c_str(), password.empty() ? nullptr : password.c_str(), &error);
    if (d) {
        out.reset(d);
        return LoadResult::Ok;
    }

    const bool encrypted = error && error->code == POPPLER_ERROR_ENCRYPTED;
    err = error ? error->message : "unknown error";
    if (error) g_error_free(error);
    return encrypted ? LoadResult::NeedPassword : LoadResult::Failed;
}

bool DocumentModel::write_copy(const std::string &file, std::string &err) const {
    // poppler_document_save() writes the document including in-memory edits
    // (annotations, form values). poppler_document_save_a_copy() silently
    // drops them in Poppler 24.02, so it must not be used here. Saving to a
    // path other than the one the document was read from is fine.
    GError *error = nullptr;
    if (!poppler_document_save(doc_.get(), util::to_uri(file).c_str(), &error)) {
        err = error ? error->message : "unknown error";
        if (error) g_error_free(error);
        return false;
    }
    return true;
}

void DocumentModel::discard_work_file() {
    if (!work_path_.empty() && work_path_ != path_) util::remove_file(work_path_);
    work_path_.clear();
}

void DocumentModel::set_dirty(bool dirty) {
    if (dirty_ == dirty) return;
    dirty_ = dirty;
    signal_dirty_changed.emit();
}

LoadResult DocumentModel::open(const std::string &path, const std::string &password,
                               std::string &err) {
    DocumentPtr loaded;
    const LoadResult result = load_file(path, password, loaded, err);
    if (result != LoadResult::Ok) return result;

    discard_work_file();
    doc_ = std::move(loaded);
    path_ = path;
    work_path_ = path;
    password_ = password;
    current_page_ = 0;
    signal_loaded.emit();
    set_dirty(false);
    return LoadResult::Ok;
}

bool DocumentModel::save(std::string &err) { return save_as(path_, err); }

bool DocumentModel::save_as(const std::string &dest, std::string &err) {
    if (!doc_) {
        err = "No document is open.";
        return false;
    }

    // Write beside the destination, then rename: a failed write can never
    // destroy the existing file.
    const std::string tmp = dest + ".efpdf-tmp";
    if (!write_copy(tmp, err)) {
        util::remove_file(tmp);
        return false;
    }
    GStatBuf st;
    if (g_stat(dest.c_str(), &st) == 0) g_chmod(tmp.c_str(), st.st_mode & 07777);
    if (g_rename(tmp.c_str(), dest.c_str()) != 0) {
        err = "Could not replace " + dest;
        util::remove_file(tmp);
        return false;
    }

    // Reload so Poppler's view of the file matches what is now on disk.
    const std::string old_work = work_path_;
    const std::string old_path = path_;
    const int page = current_page_;
    DocumentPtr reloaded;
    std::string reload_err;
    if (load_file(dest, password_, reloaded, reload_err) == LoadResult::Ok) {
        doc_ = std::move(reloaded);
        if (!old_work.empty() && old_work != old_path && old_work != dest) {
            util::remove_file(old_work);
        }
        work_path_ = dest;
    }
    path_ = dest;
    current_page_ = std::clamp(page, 0, std::max(0, page_count() - 1));
    signal_loaded.emit();
    set_dirty(false);
    return true;
}

bool DocumentModel::snapshot(std::string &tmp, std::string &err) {
    if (!doc_) {
        err = "No document is open.";
        return false;
    }
    tmp = util::make_temp_path();
    if (tmp.empty()) {
        err = "Could not create a temporary file.";
        return false;
    }
    if (!write_copy(tmp, err)) {
        util::remove_file(tmp);
        tmp.clear();
        return false;
    }
    return true;
}

bool DocumentModel::replace_with(const std::string &new_file, int page,
                                 std::string &err) {
    DocumentPtr loaded;
    if (load_file(new_file, password_, loaded, err) != LoadResult::Ok) return false;

    const std::string old_work = work_path_;
    doc_ = std::move(loaded);
    if (!old_work.empty() && old_work != path_) util::remove_file(old_work);
    work_path_ = new_file;
    current_page_ = std::clamp(page, 0, std::max(0, page_count() - 1));
    signal_loaded.emit();
    set_dirty(true);
    return true;
}

void DocumentModel::close() {
    doc_.reset();
    discard_work_file();
    path_.clear();
    password_.clear();
    current_page_ = 0;
    signal_closed.emit();
    set_dirty(false);
}

void DocumentModel::set_current_page(int page) {
    if (!doc_) return;
    page = std::clamp(page, 0, page_count() - 1);
    if (page == current_page_) return;
    current_page_ = page;
    signal_page_changed.emit();
}

void DocumentModel::notify_modified() {
    set_dirty(true);
    signal_modified.emit();
}

}  // namespace efpdf
