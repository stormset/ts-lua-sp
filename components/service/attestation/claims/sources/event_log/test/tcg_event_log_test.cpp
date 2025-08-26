/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <service/attestation/claims/sources/event_log/event_log_claim_source.h>
#include <service/attestation/claims/sources/event_log/mock/mock_event_log.h>
#include <CppUTest/TestHarness.h>

TEST_GROUP(TcgEventLogTests)
{


};

TEST(TcgEventLogTests, interateBootMeasurements)
{
    struct event_log_claim_source event_log_claim_source;
    struct claim_source *claim_source;

    claim_source = event_log_claim_source_init(&event_log_claim_source,
                                    mock_event_log_start(), mock_event_log_size());

    CHECK_FALSE(claim_source == NULL);

    struct claim boot_measurements;

    CHECK_TRUE(claim_source_get_claim(claim_source, &boot_measurements));

    UNSIGNED_LONGS_EQUAL(CLAIM_VARIANT_ID_COLLECTION, boot_measurements.variant_id);
    UNSIGNED_LONGS_EQUAL(CLAIM_SUBJECT_ID_NONE, boot_measurements.subject_id);

    /* Iterate over all claims in the collection */
    unsigned int claim_count = 0;
    unsigned int measurement_count = 0;
    struct claim_iterator iter;

    claim_collection_variant_create_iterator(&boot_measurements.variant.collection, &iter);

    while (!iter.is_done(&iter)) {

        struct claim sw_component_claim;

        ++claim_count;
        iter.current(&iter, &sw_component_claim);

        if (sw_component_claim.variant_id == CLAIM_VARIANT_ID_MEASUREMENT) {

            CHECK_TRUE(measurement_count < mock_event_Log_measurement_count());

            const struct mock_event_log_measurement *expected =
                    mock_event_Log_measurement(measurement_count);

            /* Check extracted values are as expected */
            STRCMP_EQUAL(expected->id,
                    sw_component_claim.variant.measurement.id.string);
            MEMCMP_EQUAL(expected->digest,
                    sw_component_claim.variant.measurement.digest.bytes,
                    sw_component_claim.variant.measurement.digest.len);

            ++measurement_count;
        }

        iter.next(&iter);
    }

    UNSIGNED_LONGS_EQUAL(10, claim_count);
    UNSIGNED_LONGS_EQUAL(8, measurement_count);
}
