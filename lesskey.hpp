#ifndef LESSKEY_H
#define LESSKEY_H
/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

namespace lesskey {
/*
 * Format of a lesskey file:
 *
 *    LESSKEY_MAGIC (4 bytes)
 *     sections...
 *    END_LESSKEY_MAGIC (4 bytes)
 *
 * Each section is:
 *
 *    section_MAGIC (1 byte)
 *    section_length (2 bytes)
 *    key table (section_length bytes)
 */

constexpr const char C0_LESSKEY_MAGIC = '\0';
constexpr const char C1_LESSKEY_MAGIC = 'M';
constexpr const char C2_LESSKEY_MAGIC = '+';
constexpr const char C3_LESSKEY_MAGIC = 'G';

constexpr const char CMD_SECTION  = 'c';
constexpr const char EDIT_SECTION = 'e';
constexpr const char VAR_SECTION  = 'v';
constexpr const char END_SECTION  = 'x';

constexpr const char C0_END_LESSKEY_MAGIC = 'E';
constexpr const char C1_END_LESSKEY_MAGIC = 'n';
constexpr const char C2_END_LESSKEY_MAGIC = 'd';

constexpr const int KRADIX = 64;

} // namespace lesskey

#endif
