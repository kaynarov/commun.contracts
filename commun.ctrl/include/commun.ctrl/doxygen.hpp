#pragma once

/**
 * \defgroup control c.ctrl
 * \brief This smart contract implements the logic of electing community and dApp leaders.
 * \details 
 * <b>Community leaders election</b>

 * <i>Community leader</i> — a user who is responsible for setting community parameters and moderating message content. Community leaders are elected by users of the same community through endless voting.

 * Community leaders are elected from among candidates — users who have registered themselves as candidates for leaders. A user creating the community automatically becomes its leader. However, it does not mean this user will always be on the leaderboard. 

 * A vote weight of each user depends on number of points reserved by her/him. It is assumed, the entire amount of points reserved by a user, equals to 100 % of vote. The user can split the vote into parts and use them to vote for several leaders. At any time, user can withdraw back a part of the vote from a leader and use it to vote for another leader.

 * Status of community leaders is given to candidates who received the most votes based on weight of each vote and are placed at the top of the candidates list.

 * <b>dApp leaders election</b>

 * <i>dApp leader</i> — a user who is responsible for setting application parameters and making decisions on choosing a direction of application development. dApp leaders are also elected from among candidates through endless voting.

 * Both community leaders and regular community users can vote for dApp leader. A weight of each vote depends on number of system tokens reserved on balance of the voter. The vote can also be split into parts that makes it possible to vote for several leaders.

 * Status of dApp leaders is given to the community leaders who received the most votes based on weight of each vote and are included in the number of top leadership.
 */

/**
 * \defgroup control_class Contract description
 * \ingroup control
 * \details This class implements actions providing the following:
 *  - community leader registration;
 *  - voting for community and dApp leaders;
 *  - creating specific permissions for dApp leaders.

 * Besides of it, this class contains actions providing creation and promotion of multi-signature transactions sent by leaders.
 */

/**
 * \defgroup control_tables DB Stored structures
 * \ingroup control
 * \details These structs implement the \a c.ctrl contract data stored in DB.
 */

/**
 * \defgroup control_events Events
 * \ingroup control
 * \details Events being sent to the Event-Engine by the \a c.ctrl contract.
 */
