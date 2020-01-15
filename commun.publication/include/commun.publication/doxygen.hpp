#pragma once

/**
 * \defgroup lock_actions The content blocking
 * \ingroup gallery_class
 * \details The Publication class implements actions to influence the publications whose contents are not relevant to community. These are [report][1], [lock][2], [unlock][3] and [ban][4]. Any community member can call the [report][1] action to inform a leader about an appearance of a post or comment containing inappropriate content. Leaders, having received a sufficient amount of the reports from users, decide to temporarily or permanently block such publication.

 * Below is the logic implemented in Commun that prevents manipulation of blocking competitive posts and does not allow them to be removed from the payout window:
 * - A user can perform the [report][1] action only with a signature of the client (web service).
 * - The logic analyzing a number of reports from users (and possibly automatic analysis of the report content) is fully implemented on the client side (web service).
 * - Content blocking logic implemented on the client site (web service):
 *   - The client can automatically block content by calling the [lock][2] action;
 *   - Community leaders can self-block content at any time by calling the [lock][2] action.
 * - Content blocking logic implemented on the blockchain:
 *   - Users are not allowed to make reports on a post;
 *   - The post is removed from the payout window, emission ceases to be accrued on it;
 *   - The maximum time required to unlock content is equal to moderation time.
 * - Content unblocking logic implemented on the blockchain and called with [unlock][3]:
 *   - The payout window for the post is increased by the time it is absent in this window;
 *   - Subsequent reports are prohibited unless the post has been edited by its author;
 *   - Standard logic applies to the post if it has been edited.
 * - If the post has not been unlocked by leaders during the moderation time, then the following logic applies to it:
 *   - Blockchain does not allow to unblock the post;
 *   - Funds are blocked until the end of payout and moderation windows.
 *
 * [1]: @ref commun::publication::report
 * [2]: @ref commun::publication::lock
 * [3]: @ref commun::publication::unlock
 * [4]: @ref commun::publication::ban
 */

