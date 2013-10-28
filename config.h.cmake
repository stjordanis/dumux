/* begin dumux
   put the definitions for config.h specific to
   your project here. Everything above will be
   overwritten
*/

/* begin private */
/* Name of package */
#define PACKAGE "@DUNE_MOD_NAME@"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "@DUNE_MAINTAINER@"

/* Define to the full name of this package. */
#define PACKAGE_NAME "@DUNE_MOD_NAME@"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "@DUNE_MOD_NAME@ @DUNE_MOD_VERSION@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "@DUNE_MOD_NAME@"

/* Define to the home page for this package. */
#define PACKAGE_URL "@DUNE_MOD_URL@"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@DUNE_MOD_VERSION@"

/* end private */

/* Define to 1 if dune-pdelab is patched to be usable by DuMuX */
#cmakedefine DUNE_PDELAB_IS_PATCHED_FOR_DUMUX @DUNE_PDELAB_IS_PATCHED_FOR_DUMUX@

/* Define to 1 if constexpr is supported */
#cmakedefine HAVE_CONSTEXPR @HAVE_CONSTEXPR@

/* 'set 'constexpr' to 'const' if constexpr is not supported */
#ifndef HAVE_CONSTEXPR
#define constexpr const
#endif

/* end dumux
   Everything below here will be overwritten
*/
