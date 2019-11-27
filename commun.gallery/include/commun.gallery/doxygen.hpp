#pragma once

/**
 * \defgroup gallery commun.gallery
 * \brief \a Commun.gallery is a library that provides payout information for posts and comments.
 */

/**
 * \brief The structures represent the treasures gallery descriptions stored in DB.
 * \defgroup gallery_tables DB Stored structures
 * \ingroup gallery

 * \details Treasure gallery is a collection of users' opinions about an entity that unites users of that community. The users opinions can be expressed in form of positive or negative comments, votes or other forms of attitude towards the entity presented for discussion. Each community has its own treasure gallery.
 *
 * The gallery consists of mosaics representing a separate entity. For example, a post creator creates a mosaic. Each mosaic after its creation begins to collect crystals — the opinions of curators. Each curator inserts his/her own crystal into the mosaic and thus forms a mosaic pattern.
 *
 * The mosaic pattern contains information about an author and title of post in the hash sum form. The pattern identifies the mosaic and is used as the primary key.
 *
 * Mosaic can be used as a reward for collecting any other information, not only publications. That is, it is a means to reward users who have formed a separate collection in the gallery.
 */

/**
 * \details The Events section contains description of events occurring in \a common.gallery, information about which is sent to the Event Engine.
 * \defgroup gallery_events Events
 * \ingroup gallery
 */
