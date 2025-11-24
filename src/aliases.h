#pragma once
#include <cstdint>

using time_type = uint_least32_t;

/**
 * @brief Used in distance matrix. With 16 bits, the travel time is limited to approximately 18 hours.
*/
using travel_time_type = uint_least16_t;

using delay_type = uint_least16_t;

using request_index_type = uint_fast32_t;

/**
 * @brief Type for position in plan. Negative value (-1) indicates uninitialized value
 */
using index_in_plan = int_least16_t;

/**
 * @brief Type for plan size. With 16 bits, the plan size is limited to 65535.
 */
using plan_size_type = uint_fast16_t;

/**
 * @brief id of the delayed variants in chaining algorithm. ID is unique among variants for a single plan.
 */
using variant_id_type = uint_fast16_t;
