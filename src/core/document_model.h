// document_model.h - the single source of truth for the open document.
//
// Owns the Poppler document, the current page, the "unsaved changes" flag and
// the temporary working file used by structural edits. It contains no dialogs;
// every failure is reported through an error string, and every change is
// announced through a signal so that features stay independent of each other.
#pragma once

#include <sigc++/sigc++.h>

#include <string>

#include "core/gobject_ptr.h"

namespace efpdf {

enum class LoadResult { Ok, NeedPassword, Failed };

class DocumentModel {
public:
    DocumentModel() = default;
    ~DocumentModel();
    DocumentModel(const DocumentModel &) = delete;
    DocumentModel &operator=(const DocumentModel &) = delete;

    // --- state ----------------------------------------------------------
    bool is_open() const { return doc_ != nullptr; }
    PopplerDocument *poppler() const { return doc_.get(); }
    int page_count() const;
    int current_page() const { return current_page_; }
    PagePtr page(int index) const;
    PagePtr current_page_object() const { return page(current_page_); }
    const std::string &path() const { return path_; }
    const std::string &password() const { return password_; }
    bool dirty() const { return dirty_; }

    // --- lifecycle ------------------------------------------------------
    // Opens `path`. On any failure the currently open document is untouched.
    LoadResult open(const std::string &path, const std::string &password,
                    std::string &err);
    // Writes to the opened file (via temp file + atomic rename) and reloads it.
    bool save(std::string &err);
    // Writes to `dest`, which becomes the opened file, and reloads it.
    bool save_as(const std::string &dest, std::string &err);
    // Writes the current in-memory state (annotations, form values) to a new
    // temporary file. The caller deletes it.
    bool snapshot(std::string &tmp, std::string &err);
    // Replaces the document with `new_file` (a temp file produced from a
    // snapshot). The model takes ownership of the file and marks itself dirty.
    bool replace_with(const std::string &new_file, int page, std::string &err);
    void close();

    // --- changes --------------------------------------------------------
    void set_current_page(int page);
    // Call after modifying the Poppler document in memory.
    void notify_modified();

    // --- signals --------------------------------------------------------
    sigc::signal<void> signal_loaded;        // a document was (re)loaded
    sigc::signal<void> signal_closed;        // the document was closed
    sigc::signal<void> signal_page_changed;  // current page changed
    sigc::signal<void> signal_modified;      // content changed in memory
    sigc::signal<void> signal_dirty_changed; // unsaved-changes flag changed

private:
    LoadResult load_file(const std::string &file, const std::string &password,
                         DocumentPtr &out, std::string &err) const;
    bool write_copy(const std::string &file, std::string &err) const;
    void discard_work_file();
    void set_dirty(bool dirty);

    DocumentPtr doc_;
    std::string path_;       // the file the user opened (target of Save)
    std::string work_path_;  // the file Poppler currently reads
    std::string password_;
    int current_page_ = 0;
    bool dirty_ = false;
};

}  // namespace efpdf
