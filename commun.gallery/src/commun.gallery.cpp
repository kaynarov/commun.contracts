#include "commun.gallery/commun.gallery.hpp"
#include <commun/dispatchers.hpp>

DISPATCH_WITH_TRANSFER(commun::gallery, commun::config::commun_point_name, ontransfer,
    GALLERY_ACTIONS)
