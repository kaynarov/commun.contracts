#pragma once

/**
 * \defgroup list c.list
 * \brief This smart contract contains a logic to configure communities.
 * \details The smart contract implements actions that allow a user to create a separate community, including specifying non-consensus information for the community. By default, community settings take hardcoded values. These settings can be changed by community leaders.

 * Also, the actions allow a user to receive messages only from specific authors of interest to her/him. Also, it enables community leaders to ban publications from users whose content is inconsistent with community topics.
 */

/**
 * \defgroup list_class Contract description
 * \ingroup list
 * \details The class implements actions to configure the application and create a community in it.
 */

/**
 * \defgroup list_tables DB Stored structures
 * \ingroup list
 * \details The \a c.list contract creates the records when a community is formed. The records contain a community name, token name, allowable number of leaders, rules for the distribution of awards and other information that ensures the community functioning. These records are placed in structures and stored in DB.
 */
