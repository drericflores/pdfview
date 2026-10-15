// gobject_ptr.h - RAII owners for Poppler's GObject types.
#pragma once

#include <glib-object.h>
#include <poppler.h>

#include <memory>

namespace efpdf {

struct GObjectDeleter {
    void operator()(gpointer p) const {
        if (p) g_object_unref(p);
    }
};

using DocumentPtr = std::unique_ptr<PopplerDocument, GObjectDeleter>;
using PagePtr = std::unique_ptr<PopplerPage, GObjectDeleter>;

}  // namespace efpdf
