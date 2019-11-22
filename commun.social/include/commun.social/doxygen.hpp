#pragma once

/**
 * \defgroup social commun.social
 * \brief The commun.social smart contract provides a user the following features:
 - creating and editing user's profile (metadata);
 - creating a pin-list to receive information about the publications of authors that the user is interested in;
 - creating a so-called «black» list to block unwanted users.

 <b>User profile</b> — user metadata is in the form of a structure. This  profile is created and edited by the user. The commun.social contract is not responsible for the storing of user metadata, but only controls whether the user has the right to change or delete metadata.

 <b>Pin-list</b> — a user's list of account names that a given user is interested in. This list is created and edited by the user. It can be used in the client application to create subscriptions, including informing the given user (subscriber) about the appearance of a new post whose author’s name is contained in the pin list.
 
 <b>«Black» list</b> —  a user's list of account names that this user characterizes as unwanted. The list is created and edited by its owner. «Black» list allows the user to block comments and votes of accounts whose names are contained in the list. Commun.social contract is not responsible for content of the list, as well as for its storage. The contract only controls authorization of the user when compiling the list and blocks the actions of listed accounts in relation to the user.  
 */

/**
 * \defgroup social_class Contract description
 * \ingroup social
 */
