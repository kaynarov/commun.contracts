#pragma once

/**
 * \defgroup emission c.emit
 * \brief This smart contract implements the procedure for issuing new points to communities.
 * \details The \a c.emit contract produces points for each community at a specific frequency set for that community. It accepts requests from other contracts to issue and send points to them. The number of points issuing for a requestor, depends on such parameters as annual points emission, their current number, which has already been created and sent to this requestor, as well as on frequency of requests coming from the contract. Requests may come from \a c.ctrl and \a c.gallery contracts. The generated points for \a c.ctrl contract are distributed as a reward to community leaders. The generated points for \a c.gallery contract are distributed among top mosaics.
 */

/**
 * \defgroup emission_class Contract description
 * \ingroup emission
 * \details The class contains actions that ensure an issuance of certain type points upon request from contracts.
 */

/**
 * \defgroup emission_tables DB Stored structures
 * \ingroup emission
 * \details The structures represent the records of statistical table data in DB.
 */
