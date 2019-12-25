#pragma once

/**
 * \defgroup social c.social
 * \brief This smart contract allows the user to create own profile, pin-list and «black» list.
 * \details The contract provides a user the following features:
 * - creating and editing user's profile (metadata);
 * - creating a pin-list to receive information about the publications of authors that the user is interested in;
 * - creating a so-called «black» list to block unwanted users.

 * <b>Terminology used</b>

 * <b>User profile</b> — user metadata is in the form of a structure. This  profile is created and edited by the user. The \a c.social contract is not responsible for the storing of user metadata, but only controls whether the user has the right to change or delete metadata.

 * <b>Pin-list</b> — user list of accounts that a given user is interested in. This list is created and edited by the user. It can be used in the client application to create subscriptions, including informing the given user (subscriber) about the appearance of a new post whose author’s name is contained in the pin list.

 * <b>«Black» list</b> — user list of accounts that this user characterizes as unwanted. This list is created and edited by its owner. «Black» list allows the user to block comments and votes of accounts whose names are contained in the list. The \a c.social contract is not responsible for content of the list, as well as for its storage. The contract only controls authorization of the user when compiling the list.
 */

/**
 * \defgroup social_class Contract description
 * \ingroup social
 * \details The class implements actions allows a user to create own profile, pin-list and "black" list.
 */
