#pragma once

/**
 * \defgroup gallery c.gallery
 * \brief This smart contract looks like a library that provides payout information for posts and comments.
 * \details Treasure gallery is a collection of users' opinions about an entity that unites users of that community. The users opinions can be expressed in form of positive or negative comments, votes or other forms of attitude towards the entity presented for discussion. Each community has its own treasure gallery.

 * The gallery consists of mosaics representing a separate entity. For example, a post creator creates a mosaic. Each mosaic after its creation begins to collect gems — the opinions of curators. Each curator inserts his/her own gem into the mosaic and thus forms its total weight.

 * The mosaic tracery contains information about an author and title of post in the hash sum form. The tracery identifies the mosaic and is used as the primary key.

 * Mosaic can be used as a reward for collecting any other information, not only publications. That is, it is a means to reward users who have formed a separate collection in the gallery.
 */

/**
 * \defgroup gallery_class Contract description
 * \ingroup gallery
 * \details This class implements the actions that allow a user:
 *  - to create and edit a message;
 *  - to vote for other messages;
 *  - to report about undesirable or suspicious messages to leaders.

 * Also, the actions allow the community leaders:
 *  - to lock a message whose contents is not relevant to community;
 *  - to configure a gallery for community; 
 *  - to modify tags of messages.
 */

/**
 * \defgroup gallery_tables DB Stored structures
 * \ingroup gallery
 * \details The structures represent the treasures gallery descriptions stored in DB.
 */

/**
 * \defgroup gallery_events Events
 * \ingroup gallery
 * \details The Events section contains description of events occurring in \a c.gallery, information about which is sent to the Event Engine.
 */
