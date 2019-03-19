/*
 * Copyright (c) 2018 Nitrokey UG
 *
 * This file is part of Nitrokey HOTP verification project.
 *
 * Nitrokey HOTP verification is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey HOTP verification is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "catch.hpp"

extern "C" {
  #include "../operations.h"
}

const char* base32_secret = "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ";
const char* admin_PIN = "12345678";
const char* RFC_HOTP_codes[] = {
  "755224", //0
  "287082",
  "359152",
  "969429", //3
  "338314",
  "254676",
  "287922", //6
  "162583",
  "399871",
  "520489", //9
  "403154", //10
  "481090", //11
};

struct Device dev;
const char * key_brand = "Test device";


TEST_CASE("Test correct codes", "[HOTP]") {
  int res;
  res = device_connect(&dev, key_brand);
  REQUIRE(res == true);
  res = set_secret_on_device(&dev, base32_secret, admin_PIN);
  REQUIRE(res == RET_NO_ERROR);
  for (auto c: RFC_HOTP_codes){
    res = check_code_on_device(&dev, c);
    REQUIRE(res == RET_VALIDATION_PASSED);
  }
  device_disconnect(&dev);
}

TEST_CASE("Test incorrect codes", "[HOTP]") {
  int res;
  res = device_connect(&dev, key_brand);
  REQUIRE(res == true);
  res = set_secret_on_device(&dev, base32_secret, admin_PIN);
  REQUIRE(res == RET_NO_ERROR);

  for (int i=0; i<10; i++){
    res = check_code_on_device(&dev, "123456");
    REQUIRE(res == RET_VALIDATION_FAILED);
  }

  device_disconnect(&dev);
}


TEST_CASE("Test codes with offset 2", "[HOTP]") {
  int res;
  res = device_connect(&dev, key_brand);
  REQUIRE(res == true);
  res = set_secret_on_device(&dev, base32_secret, admin_PIN);
  REQUIRE(res == RET_NO_ERROR);

  int i=0;
  for (auto c: RFC_HOTP_codes){
    if (i++%2 == 0) continue;
    res = check_code_on_device(&dev, c);
    REQUIRE(res == RET_VALIDATION_PASSED);
  }

  device_disconnect(&dev);
}

TEST_CASE("Test code with maximum offsets", "[HOTP]") {
  int res;
  res = device_connect(&dev, key_brand);
  REQUIRE(res == true);
  res = set_secret_on_device(&dev, base32_secret, admin_PIN);
  REQUIRE(res == RET_NO_ERROR);


  res = check_code_on_device(&dev, RFC_HOTP_codes[11]);
  REQUIRE(res == RET_VALIDATION_FAILED);
  res = check_code_on_device(&dev, RFC_HOTP_codes[10]);
  REQUIRE(res == RET_VALIDATION_FAILED);

  res = check_code_on_device(&dev, RFC_HOTP_codes[9]);
  REQUIRE(res == RET_VALIDATION_PASSED);
  res = check_code_on_device(&dev, RFC_HOTP_codes[11]);
  REQUIRE(res == RET_VALIDATION_PASSED);


  res = device_disconnect(&dev);
  REQUIRE(res == RET_NO_ERROR);
}


#include "../device.h"

TEST_CASE("Try to set the HOTP secret with wrong PIN and test PIN counters", "[HOTP]") {
  int res;
  res = device_connect(&dev, key_brand);
  REQUIRE(res == true);

  struct ResponseStatus status = device_get_status(&dev);
  REQUIRE(status.retry_admin >= 2);

  res = set_secret_on_device(&dev, base32_secret, admin_PIN);
  REQUIRE(res == RET_NO_ERROR);
  status = device_get_status(&dev);
  REQUIRE(status.retry_admin == 3);
  REQUIRE(check_code_on_device(&dev, RFC_HOTP_codes[0]) == RET_VALIDATION_PASSED);

  res = set_secret_on_device(&dev, base32_secret, "wrong_PIN");
  REQUIRE(res == dev_wrong_password);
  status = device_get_status(&dev);
  REQUIRE(status.retry_admin == 2);
  REQUIRE(check_code_on_device(&dev, RFC_HOTP_codes[0]) == RET_VALIDATION_FAILED);

  res = set_secret_on_device(&dev, base32_secret, admin_PIN);
  REQUIRE(res == RET_NO_ERROR);
  status = device_get_status(&dev);
  REQUIRE(status.retry_admin == 3);

  for (auto c: RFC_HOTP_codes){
    res = check_code_on_device(&dev, c);
    REQUIRE(res == RET_VALIDATION_PASSED);
  }

  res = device_disconnect(&dev);
  REQUIRE(res == RET_NO_ERROR);
}

TEST_CASE("Verify base32 string", "[Helper]") {
  std::string invalid_base32 = "111";
  std::string valid_base32 = "AAAAA";
  REQUIRE_FALSE(verify_base32(invalid_base32.c_str(), invalid_base32.length()));
  REQUIRE(verify_base32(valid_base32.c_str(), valid_base32.length()));
}