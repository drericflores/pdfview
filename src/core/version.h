// version.h - release number (shown in Help > About) and the git revision
// the binary was built from (see Makefile; shown by `efpdf --version`).
#pragma once

namespace efpdf {
inline constexpr const char *kRelease = "0.4.1";
const char *version();  // git revision, for logs and --version only
}  // namespace efpdf
