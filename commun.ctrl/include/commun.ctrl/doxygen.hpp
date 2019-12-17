#pragma once

/**
 * \defgroup control commun.ctrl
 * \brief This smart contract implements the logic of electing and rewarding community leaders.
 * \details <i>Community leader</i> — a user who is responsible for setting community parameters and moderating message content. Leaders are selected by community users through endless voting.

 * A user creating the community automatically becomes its leader. However, it does not mean this user will always be on the leaderboard. Voting for leaders lasts all the time. Leaders are selected from among candidates — users who have registered themselves as candidates for leaders.

 * A vote weight of each user depends on number of points reserved by her/him. It is assumed, the entire amount of reserved points reserved by user, equals to 100 % of vote. User can split the vote into parts and use them for several leaders. At any time user can withdraw back the vote from a leader and vote with it for another leader.

 * Community leaders are candidates who received the maximum number of votes and included in the top list of leaders.
 */

/**
 * \defgroup control_class Contract description
 * \ingroup control
 * \details This class implements actions providing the following:
 *  - community leader registration;
 *  - voting for community leaders;
 *  - rewarding community leaders.

 * Besides of it, this class contains actions providing creation and promotion of multi-signature transactions.
 */

/**
 * \defgroup control_tables DB Stored structures
 * \ingroup control
 * \details These structs implement the \a commun.ctrl contract data stored in DB.
 */

/**
 * \defgroup control_events Events
 * \ingroup control
 * \details Events being sent to the Event-Engine by the \a commun.ctrl contract.
 */
