#pragma once

/**
 * \defgroup recover_grp c.recover
 * \brief This smart contract contains a logic to recover user-authority (active & owner).
 * \details The smart contract implements actions that allow a user to recover active & owner authorities through client.
 */

/**
 * \defgroup recover_class Contract description
 * \ingroup recover_grp
 * \details The class implements actions to recover a user-authority (active & owner) through client.
 */

/**
 * \defgroup recover_tables DB Stored structures
 * \ingroup recover_grp
 * \details The c.recover contract creates the records in the database when the user requests recovery of owner-authority. Information about pending requests contains new owner-key and time when this key can be applied.
 */
