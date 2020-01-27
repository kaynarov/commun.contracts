#pragma once


/**
 * \mainpage Introduction
 
 * \section intro1 Overview
 Commun is a platform for building autonomous self-governing communities based on smart contracts of the CyberWay Blockchain. Everybody can feel free to join the Commun. Users can create their own separately monetized Communities and set their own rules within ones.

 * Our mission is to allow people to create self-managed online communities that should be easily monetized by users.
Such a goal can be achieved if the following conditions are met:
 * - User data management is verifiable by users themselves;
 * - User data security services are verifiable and manageable by users themselves;
 * - Community affiliation is not enforced by technical or financial bounds.


 * \subsection intro11 Commun Smart Contracts
 Commun is a set of interconnected smart contracts impemented in CyberWay blockchain. These contracts provide for the creation of communities, interaction between communities and their members on the one hand and CyberWay blockchain on the other. All smart contracts are not considered unique to each community. Number of contracts is unlimited and may increase as necessary.

 * \subsection intro_sec12 Community as a collections gallery
 Main currency in the commun application is the CMN token. Main currency in a community is a point. Each community has its own type of the point. All point-token exchanges are carried out through the intermediary \a c.point contract. The exchange of tokens for points is carried out at the current rate.

 * A community publishes its materials in accordance with its chosen topic, forming a collection of publications — a gallery of treasures. Each community has its own gallery of treasures (e.g., a gallery for gardeners, a gallery for fishing enthusiasts, etc.). The community collects user opinions that reflect a treasure of the collection. User opinions form a mosaic, each element of which (a gem) reflects the opinion of an individual user.

 * The gallery consists of mosaics, each of which is created by an author. Creating a post, author automatically creates a mosaic. The mosaic is identified by a tracery that is the hash sum generated from author name and post title. The tracery is used as the primary key.

 * Once a mosaic is created, it begins to collect gems. A curator inserts his/her own gem into the mosaic when leaves his/her own opinion on publication. During voting for a post, a curator votes by points, which determine a weight of the gem is leaving in the mosaic. Part of the weight is allocated to author of the mosaic.

 * Mosaic can be used as a reward for collecting any other information, not just publishing a post.

 * The logic containing the publication hierarchy is implemented in the commun.publication component while the rewarding posts logic for authors and curators is implemented separately and placed in the \a c.gallery contract. This contract looks like a library for rewards of unique collections, not only publications.

 * \section intro_sec2 Terminology used
 * <i>Commun leader</i> or <i>dApp leader</i> — a person registered in the Commun application as a leader and is on the commun top-leaders list. Commun leader is responsible for setting application parameters and making decisions on choosing a direction of application development. Commun leader is elected by community leaders and users through endless voting.

 * \a Community — a group of persons formed around some point of interest. Community can be created by any user or group of users. Each community has its own point and parameters.

 * <i>Community leader</i> — a person registered in the Commun application as a leader and is on top-leaders list of the same community. Leader is responsible for setting community parameters and moderating message content. Community leaders are elected by users of the same community through endless voting.

 * \a <i>Community user</i> — a person is considered a community user if the person is registered in the Commun application and has a balance with points of same community. A user can be a member of several communities simultaneously if the user has balances with points of the same communities. Community user able to perform the following operations: posting, voting for a content and a leader, creating a community.

 * \a Gallery — a collection of mosaics united by the same theme. Each community has its own gallery.

 * \a Gem — user's opinion about an entity presented for discussion. Opinion can be expressed in the form of positive or negative comment, vote or other forms of attitude to the entity. The strength (weight) of opinion depends on number of points allocated to it by the user.

 * \a Mosaic — data table of the entity discussion process from the moment of its publication to the reward completion moment to all participants in this process. An author at the time of publication creates a mosaic and inserts the first gem into it. Each user, leaving a comment or voting for the post, contributes a gem to this mosaic. Once the discussion and payments are terminated, this mosaic collapses.

 * \a Opus — mosaic description type. It indicates what the mosaic describes, such as a post or comment.

 * \a Point — main payment unit used for payments within a separate community. Point is not a token and can not be used in exchange trading. Points are used as coins to encourage community members and applied only within the community.

 * \a Safe — functionality allowing a user to manipulate funds in order to preserve them, namely to lock funds and withdrawal operations on them, including transfer, selling points to buy tokens. Access to funds is possible only after they are unlocked by the safe owner.

 * <i>Token CMN</i> (abbr. from Commun) or <i>System token</i> — main coin registered in the system of CyberWay and introduced to exchange of funds between communities, as well as between Commun and Cyberway blockchain.

 * \a Tracery — mosaic identifier, which is a hash sum generated from post author name and publication title.

 * <i>Trusted community client</i> — a person appointed by commun leaders to make decisions within the dApp application.
 */