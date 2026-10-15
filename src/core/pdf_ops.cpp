#include "core/pdf_ops.h"

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>

#include <exception>
#include <memory>
#include <numeric>

namespace efpdf::pdf_ops {
namespace {

void write_file(QPDF &pdf, const std::string &out) {
    QPDFWriter writer(pdf, out.c_str());
    writer.write();
}

void read_file(QPDF &pdf, const std::string &in, const std::string &password) {
    pdf.processFile(in.c_str(), password.empty() ? nullptr : password.c_str());
}

bool check_index(int page, size_t count, std::string &err) {
    if (page < 0 || static_cast<size_t>(page) >= count) {
        err = "Page number is out of range.";
        return false;
    }
    return true;
}

}  // namespace

bool rotate_page(const std::string &in, const std::string &out, int page,
                 int degrees, const std::string &password, std::string &err) {
    try {
        QPDF pdf;
        read_file(pdf, in, password);
        QPDFPageDocumentHelper doc(pdf);
        auto pages = doc.getAllPages();
        if (!check_index(page, pages.size(), err)) return false;

        // Normalise to 0/90/180/270 so qpdf never sees a negative angle.
        const int angle = ((degrees % 360) + 360) % 360;
        pages[page].rotatePage(angle, true);
        write_file(pdf, out);
        return true;
    } catch (const std::exception &e) {
        err = e.what();
        return false;
    }
}

bool delete_page(const std::string &in, const std::string &out, int page,
                 const std::string &password, std::string &err) {
    try {
        QPDF pdf;
        read_file(pdf, in, password);
        QPDFPageDocumentHelper doc(pdf);
        auto pages = doc.getAllPages();
        if (!check_index(page, pages.size(), err)) return false;
        if (pages.size() < 2) {
            err = "A PDF must keep at least one page.";
            return false;
        }
        doc.removePage(pages[page]);
        write_file(pdf, out);
        return true;
    } catch (const std::exception &e) {
        err = e.what();
        return false;
    }
}

bool move_page(const std::string &in, const std::string &out, int from, int to,
               const std::string &password, std::string &err) {
    try {
        QPDF pdf;
        read_file(pdf, in, password);
        QPDFPageDocumentHelper doc(pdf);
        auto pages = doc.getAllPages();
        if (!check_index(from, pages.size(), err) ||
            !check_index(to, pages.size(), err)) {
            return false;
        }

        std::vector<int> order(pages.size());
        std::iota(order.begin(), order.end(), 0);
        order.erase(order.begin() + from);
        order.insert(order.begin() + to, from);

        for (auto &p : pages) doc.removePage(p);
        for (int idx : order) doc.addPage(pages[idx], false);
        write_file(pdf, out);
        return true;
    } catch (const std::exception &e) {
        err = e.what();
        return false;
    }
}

bool merge(const std::vector<std::string> &inputs, const std::string &out,
           std::string &err) {
    if (inputs.empty()) {
        err = "No input files were given.";
        return false;
    }
    try {
        // Source files must outlive the write, because foreign pages are
        // copied lazily when the output is written.
        std::vector<std::unique_ptr<QPDF>> sources;
        QPDF merged;
        merged.emptyPDF();
        QPDFPageDocumentHelper merged_doc(merged);

        for (const auto &file : inputs) {
            sources.push_back(std::make_unique<QPDF>());
            sources.back()->processFile(file.c_str());
            QPDFPageDocumentHelper src_doc(*sources.back());
            for (auto &page : src_doc.getAllPages()) {
                merged_doc.addPage(page, false);
            }
        }
        write_file(merged, out);
        return true;
    } catch (const std::exception &e) {
        err = e.what();
        return false;
    }
}

bool extract_range(const std::string &in, const std::string &out, int first,
                   int last, const std::string &password, std::string &err) {
    try {
        QPDF source;
        read_file(source, in, password);
        QPDFPageDocumentHelper source_doc(source);
        auto pages = source_doc.getAllPages();
        if (!check_index(first, pages.size(), err) ||
            !check_index(last, pages.size(), err)) {
            return false;
        }
        if (first > last) {
            err = "The first page must not be after the last page.";
            return false;
        }

        QPDF result;
        result.emptyPDF();
        QPDFPageDocumentHelper result_doc(result);
        for (int i = first; i <= last; ++i) result_doc.addPage(pages[i], false);
        write_file(result, out);
        return true;
    } catch (const std::exception &e) {
        err = e.what();
        return false;
    }
}

}  // namespace efpdf::pdf_ops
