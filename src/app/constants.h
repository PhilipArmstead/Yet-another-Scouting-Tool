// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types/position.h"

#define MINIMUM_POSITIONAL_PROFICIENCY 15
#define MAX_RATING_VALUE 120.f

#define RESOURCE_BASE "/com/philarmstead/fmplayerrater"

// All *_PTR_BASE values are relative to the base address of the module (fm.exe)

#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE 0x0642EE60
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_1 0xB78
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_2 0x28
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_3 0x48
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_4 0x328
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_5 0x0
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_6 0x28
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_7 0x98
#define CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_8 0x148

#define CURRENT_SCREEN_STAFF_ID_PTR_BASE 0x06374408
#define CURRENT_SCREEN_STAFF_ID_PTR_BASE_OFFSET_1 0x18
#define CURRENT_SCREEN_STAFF_ID_PTR_BASE_OFFSET_2 0x10

#define GAME_VERSION_PTR_BASE 0x063659C0
#define GAME_VERSION_PTR_OFFSET_1 0x88
#define GAME_VERSION_PTR_OFFSET_2 0x04

#define CLUB_LIST_PTR_BASE 0x0642ECE0
#define CLUB_LIST_PTR_BASE_OFFSET 0x80
#define CLUB_LIST_START 0x00
#define CLUB_LIST_END 0x08
#define CLUB_LIST_STRIDE 0x08
// Jump to CLUB_LIST_STRIDE * index to get the club address

#define COMPETITION_LIST_PTR_BASE 0x0642ECE8
#define COMPETITION_LIST_PTR_OFFSET_1 0x80
#define COMPETITION_LIST_PTR_OFFSET_2 0x00
#define COMPETITION_LIST_PTR_OFFSET_3 0x08
// Jump to COMPETITION_LIST_PTR_OFFSET_3 * index to get the competition address

#define CONTINENT_LIST_PTR_BASE 0x0642ECF0
#define CONTINENT_LIST_PTR_OFFSET_1 0x80
#define CONTINENT_LIST_PTR_OFFSET_2 0x00
#define CONTINENT_LIST_PTR_OFFSET_3 0x08
// Jump to CONTINENT_LIST_PTR_OFFSET_3 * index to get the continent address

#define NATION_LIST_PTR_BASE 0x0642ECF8
#define NATION_LIST_PTR_BASE_OFFSET 0x80
#define NATION_LIST_START 0x00
#define NATION_LIST_END 0x08
#define NATION_LIST_STRIDE 0x08
// Jump to NATION_LIST_STRIDE * index to get the nation address

#define INJURIES_LIST_PTR_BASE 0x0642ED10
#define INJURIES_LIST_PTR_OFFSET_1 0x80
#define INJURIES_LIST_PTR_OFFSET_2 0x00
#define INJURIES_LIST_PTR_OFFSET_3 0x08
// Jump to INJURIES_LIST_PTR_OFFSET_3 * index to get the injury address

// When a person is _only_ a player, their details are at 0x278
// When a person is player + staff, staff is at 0x278 and player is at 0x368
// TODO: find a way to determine if a person is player + staff or just player
#define PLAYER_OFFSET_FROM_PERSON (-0x278)
#define STAFF_OFFSET_FROM_PERSON (-0x368)

#define PLAYER_COUNT_PTR_BASE 0x642C034
#define PLAYER_LIST_PTR_BASE 0x0642EDD0
#define PLAYER_LIST_STRIDE 0x08
// Jump to PLAYER_LIST_STRIDE * index to get the player address

#define PEOPLE_LIST_PTR_BASE 0x0642ED38
#define PEOPLE_LIST_PTR_OFFSET_1 0x80
#define PEOPLE_LIST_PTR_OFFSET_2 0x00
#define PEOPLE_LIST_PTR_OFFSET_3 0x08
// Jump to PEOPLE_LIST_PTR_OFFSET_3 * index to get the person address

#define TEAM_LIST_PTR_BASE 0x0642ED68
#define TEAM_LIST_PTR_OFFSET_1 0x80
#define TEAM_LIST_PTR_OFFSET_2 0x00
#define TEAM_LIST_PTR_OFFSET_3 0x08
// Jump to TEAM_LIST_PTR_OFFSET_3 * index to get the team address

#define CLUB_LIST_PTR_BASE 0x0642ECE0
#define CLUB_LIST_PTR_OFFSET_1 0x80
#define CLUB_LIST_PTR_OFFSET_2 0x00
#define CLUB_LIST_PTR_OFFSET_3 0x08
// Jump to CLUB_LIST_PTR_OFFSET_3 * index to get the club address
// Longest club name: Club de FÃºtbol Lobos de la BenemÃ©rita Universidad AutÃ³noma de Puebla, 71 chars
// Longest club short name: Persatuan Sepakbola Indonesia Karawang, 38 chars

#define COMPETITION_LIST_PTR_BASE 0x0642ECE8
#define COMPETITION_LIST_PTR_OFFSET_1 0x80
#define COMPETITION_LIST_PTR_OFFSET_2 0x00
#define COMPETITION_LIST_PTR_OFFSET_3 0x08
// Jump to COMPETITION_LIST_PTR_OFFSET_3 * index to get the competition address

// Day is represented with 1 byte and a 1 bit in the following byte to represent values > 255
// Time is represented as 1 byte (even bits only) determining the number of 15 minute periods since 6AM
// (plus 1 because 0 is midnight)
// Year is 2 bytes

// e.g. Sunday 12th of May 2024 at 0800 is 85 12 E8 07
#define CURRENT_DATETIME_PTR_BASE 0x631d5bc

/** Person definition */
// The next person is +0x3e8 bytes away
#define PERSON_OFFSET_ROW_ID 0x08
#define PERSON_OFFSET_UNIQUE_ID 0x0C
#define PERSON_OFFSET_RANDOM_ID 0x10
// One byte: 0x02 = male, 0x12 = female
#define PERSON_OFFSET_GENDER 0x19
// 0x40 B8 1A E7 07 Some sort of date?
// 2 bytes for day of the year, 2 bytes for year
#define PERSON_OFFSET_DOB 0x44
// (+0x04 from here)
// Longest common name: 23 chars, Valentín Mariano José
#define PERSON_OFFSET_FORENAME 0x58
// (+0x04 from here)
// Longest common name: 32 chars, Aparecido Leite de Souza Júnior
#define PERSON_OFFSET_SURNAME 0x60
// (+0x04 from here)
// Longest common name: 30 chars, Steinþór Freyr Þorsteinsson
#define PERSON_OFFSET_COMMON_NAME 0x68
#define PERSON_OFFSET_NATIONALITY 0x70
#define PERSON_OFFSET_PERSONALITY 0x78
#define PERSON_OFFSET_ADAPTABILITY 0x78
#define PERSON_OFFSET_AMBITION 0x79
#define PERSON_OFFSET_LOYALTY 0x7A
#define PERSON_OFFSET_PRESSURE 0x7B
#define PERSON_OFFSET_PROFESSIONALISM 0x7C
#define PERSON_OFFSET_SPORTSMANSHIP 0x7D
#define PERSON_OFFSET_TEMPERAMENT 0x7E
#define PERSON_OFFSET_CONTROVERSY 0x7F
// (0x00 from here is start, 0x08 from here is end)
#define PERSON_OFFSET_RELATIONSHIPS 0x80
#define PERSON_OFFSET_CITY_OF_BIRTH 0x88
#define PERSON_OFFSET_CONTRACTS 0xC8

/** Player definition */
// (0x00 from here is start, 0x08 from here is end)
#define PLAYER_OFFSET_INJURY_POINTER 0xF8
#define PLAYER_OFFSET_GUIDE_VALUE 0x1D0
#define PLAYER_OFFSET_TRANSFER_VALUE 0x1D4
#define PLAYER_OFFSET_SHARPNESS 0x1F4
// negative for some reason; 0 is best
#define PLAYER_OFFSET_FATIGUE 0x1F6
#define PLAYER_OFFSET_CONDITION 0x1F8
#define PLAYER_OFFSET_HOME_REPUTATION 0x1FA
#define PLAYER_OFFSET_CURRENT_REPUTATION 0x1FC
#define PLAYER_OFFSET_WORLD_REPUTATION 0x1FE
#define PLAYER_OFFSET_ABILITY 0x200
#define PLAYER_OFFSET_POTENTIAL_ABILITY 0x202
#define PLAYER_OFFSET_POSITIONS 0x208
#define PLAYER_OFFSET_POSITION_GK (PLAYER_OFFSET_POSITIONS + POSITION_CODE_GK)
#define PLAYER_OFFSET_POSITION_SW (PLAYER_OFFSET_POSITIONS + POSITION_CODE_SW)
#define PLAYER_OFFSET_POSITION_DL (PLAYER_OFFSET_POSITIONS + POSITION_CODE_DL)
#define PLAYER_OFFSET_POSITION_DC (PLAYER_OFFSET_POSITIONS + POSITION_CODE_DC)
#define PLAYER_OFFSET_POSITION_DR (PLAYER_OFFSET_POSITIONS + POSITION_CODE_DR)
#define PLAYER_OFFSET_POSITION_DM (PLAYER_OFFSET_POSITIONS + POSITION_CODE_DM)
#define PLAYER_OFFSET_POSITION_ML (PLAYER_OFFSET_POSITIONS + POSITION_CODE_ML)
#define PLAYER_OFFSET_POSITION_MC (PLAYER_OFFSET_POSITIONS + POSITION_CODE_MC)
#define PLAYER_OFFSET_POSITION_MR (PLAYER_OFFSET_POSITIONS + POSITION_CODE_MR)
#define PLAYER_OFFSET_POSITION_AML (PLAYER_OFFSET_POSITIONS + POSITION_CODE_AML)
#define PLAYER_OFFSET_POSITION_AMC (PLAYER_OFFSET_POSITIONS + POSITION_CODE_AMC)
#define PLAYER_OFFSET_POSITION_AMR (PLAYER_OFFSET_POSITIONS + POSITION_CODE_AMR)
#define PLAYER_OFFSET_POSITION_ST (PLAYER_OFFSET_POSITIONS + POSITION_CODE_ST)
#define PLAYER_OFFSET_POSITION_WBL (PLAYER_OFFSET_POSITIONS + POSITION_CODE_WBL)
#define PLAYER_OFFSET_POSITION_WBR (PLAYER_OFFSET_POSITIONS + POSITION_CODE_WBR)
#define PLAYER_OFFSET_ATTRIBUTES 0x217
#define PLAYER_OFFSET_ATTRIBUTE_CROSSING 0x217
#define PLAYER_OFFSET_ATTRIBUTE_DRIBBLING 0x218
#define PLAYER_OFFSET_ATTRIBUTE_FINISHING 0x219
#define PLAYER_OFFSET_ATTRIBUTE_HEADING 0x21A
#define PLAYER_OFFSET_ATTRIBUTE_LONG_SHOTS 0x21B
#define PLAYER_OFFSET_ATTRIBUTE_MARKING 0x21C
#define PLAYER_OFFSET_ATTRIBUTE_OTB 0x21D
#define PLAYER_OFFSET_ATTRIBUTE_PASSING 0x21E
#define PLAYER_OFFSET_ATTRIBUTE_PENALTY 0x21F
#define PLAYER_OFFSET_ATTRIBUTE_TACKLING 0x220
#define PLAYER_OFFSET_ATTRIBUTE_VISION 0x221
#define PLAYER_OFFSET_ATTRIBUTE_HANDLING 0x222
#define PLAYER_OFFSET_ATTRIBUTE_AERIAL_ABILITY 0x223
#define PLAYER_OFFSET_ATTRIBUTE_COMMAND 0x224
#define PLAYER_OFFSET_ATTRIBUTE_COMMUNICATION 0x225
#define PLAYER_OFFSET_ATTRIBUTE_KICKING 0x226
#define PLAYER_OFFSET_ATTRIBUTE_THROWING 0x227
#define PLAYER_OFFSET_ATTRIBUTE_ANTICIPATION 0x228
#define PLAYER_OFFSET_ATTRIBUTE_DECISIONS 0x229
#define PLAYER_OFFSET_ATTRIBUTE_ONE_ON_ONES 0x22A
#define PLAYER_OFFSET_ATTRIBUTE_POSITIONING 0x22B
#define PLAYER_OFFSET_ATTRIBUTE_REFLEXES 0x22C
#define PLAYER_OFFSET_ATTRIBUTE_FIRST_TOUCH 0x22D
#define PLAYER_OFFSET_ATTRIBUTE_TECHNIQUE 0x22E
#define PLAYER_OFFSET_ATTRIBUTE_FOOT_LEFT 0x22F
#define PLAYER_OFFSET_ATTRIBUTE_FOOT_RIGHT 0x230
#define PLAYER_OFFSET_ATTRIBUTE_FLAIR 0x231
#define PLAYER_OFFSET_ATTRIBUTE_CORNERS 0x232
#define PLAYER_OFFSET_ATTRIBUTE_TEAMWORK 0x233
#define PLAYER_OFFSET_ATTRIBUTE_WORK_RATE 0x234
#define PLAYER_OFFSET_ATTRIBUTE_LONG_THROWS 0x235
#define PLAYER_OFFSET_ATTRIBUTE_ECCENTRICITY 0x236
#define PLAYER_OFFSET_ATTRIBUTE_RUSHING_OUT 0x237
#define PLAYER_OFFSET_ATTRIBUTE_PUNCHING 0x238
#define PLAYER_OFFSET_ATTRIBUTE_ACCELERATION 0x239
#define PLAYER_OFFSET_ATTRIBUTE_FREE_KICKS 0x23A
#define PLAYER_OFFSET_ATTRIBUTE_STRENGTH 0x23B
#define PLAYER_OFFSET_ATTRIBUTE_STAMINA 0x23C
#define PLAYER_OFFSET_ATTRIBUTE_PACE 0x23D
#define PLAYER_OFFSET_ATTRIBUTE_JUMPING 0x23E
#define PLAYER_OFFSET_ATTRIBUTE_LEADERSHIP 0x23F
#define PLAYER_OFFSET_ATTRIBUTE_DIRTINESS 0x240
#define PLAYER_OFFSET_ATTRIBUTE_BALANCE 0x241
#define PLAYER_OFFSET_ATTRIBUTE_BRAVERY 0x242
#define PLAYER_OFFSET_ATTRIBUTE_CONSISTENCY 0x243
#define PLAYER_OFFSET_ATTRIBUTE_AGGRESSION 0x244
#define PLAYER_OFFSET_ATTRIBUTE_AGILITY 0x245
#define PLAYER_OFFSET_ATTRIBUTE_IMPORTANT_MATCHES 0x246
#define PLAYER_OFFSET_ATTRIBUTE_INJURY_PRONENESS 0x247
#define PLAYER_OFFSET_ATTRIBUTE_VERSATILITY 0x248
#define PLAYER_OFFSET_ATTRIBUTE_NAT_FITNESS 0x249
#define PLAYER_OFFSET_ATTRIBUTE_DETERMINATION 0x24A
#define PLAYER_OFFSET_ATTRIBUTE_COMPOSURE 0x24B
#define PLAYER_OFFSET_ATTRIBUTE_CONCENTRATION 0x24C
#define PLAYER_OFFSET_HIDDEN_ATTRIBUTES 0x246
// 0x01 = right
// 0x02 = left
// 0x03 = right/central
// 0x04 = left/central
// 0x05 = central
// 0x06 = right when 2, right/central
#define PLAYER_OFFSET_PREFERRED_POSITION 0x266

// Attribute indices relative to PLAYER_OFFSET_ATTRIBUTES (0x217)
#define ATTR_CRO 0 // Crossing
#define ATTR_DRI 1 // Dribbling
#define ATTR_FIN 2 // Finishing
#define ATTR_HEA 3 // Heading
#define ATTR_LON 4 // LongShots
#define ATTR_MAR 5 // Marking
#define ATTR_OTB 6 // OffTheBall
#define ATTR_PAS 7 // Passing
#define ATTR_PEN 8 // PenaltyTaking
#define ATTR_TCK 9 // Tackling
#define ATTR_VIS 10 // Vision
#define ATTR_HAN 11 // Handling
#define ATTR_AER 12 // AerialReach
#define ATTR_CMD 13 // CommandOfArea
#define ATTR_COM 14 // Communication
#define ATTR_KIC 15 // Kicking
#define ATTR_THR 16 // Throwing
#define ATTR_ANT 17 // Anticipation
#define ATTR_DEC 18 // Decisions
#define ATTR_ONE 19 // OneOnOnes
#define ATTR_POS 20 // Positioning
#define ATTR_REF 21 // Reflexes
#define ATTR_FIR 22 // FirstTouch
#define ATTR_TEC 23 // Technique
#define ATTR_LEF 24 // LeftFoot
#define ATTR_RIG 25 // RightFoot
#define ATTR_FLA 26 // Flair
#define ATTR_COR 27 // CornerTaking
#define ATTR_TEA 28 // Teamwork
#define ATTR_WOR 29 // WorkRate
#define ATTR_LTH 30 // LongThrows
#define ATTR_ECC 31 // Eccentricity
#define ATTR_TRO 32 // RushingOut
#define ATTR_TTP 33 // Punching
#define ATTR_ACC 34 // Acceleration
#define ATTR_FRE 35 // FreeKickTaking
#define ATTR_STR 36 // Strength
#define ATTR_STA 37 // Stamina
#define ATTR_PAC 38 // Pace
#define ATTR_JUM 39 // JumpingReach
#define ATTR_LDR 40 // Leadership
#define ATTR_DIR 41 // Dirtiness
#define ATTR_BAL 42 // Balance
#define ATTR_BRA 43 // Bravery
#define ATTR_CON 44 // Consistency
#define ATTR_AGG 45 // Aggression
#define ATTR_AGI 46 // Agility
#define ATTR_IMP 47 // ImportantMatches
#define ATTR_INJ 48 // InjuryProneness
#define ATTR_VER 49 // Versatility
#define ATTR_NAT 50 // NaturalFitness
#define ATTR_DET 51 // Determination
#define ATTR_CMP 52 // Composure
#define ATTR_CNT 53 // Concentration
#define ATTR_ADA 54 // Adaptability
#define ATTR_AMB 55 // Ambition
#define ATTR_LOY 56 // Loyalty
#define ATTR_PRE 57 // Pressure
#define ATTR_PRO 58 // Professionalism
#define ATTR_SPO 59 // Sportsmanship
#define ATTR_TEM 60 // Temperament
#define ATTR_CNY 61 // Controversy
#define ATTRIBUTE_COUNT (ATTR_CNY + 1)
#define PERSONALITY_COUNT 8
#define TRUE_ATTRIBUTE_COUNT (ATTRIBUTE_COUNT - PERSONALITY_COUNT)

static const char *attributeNames[ATTRIBUTE_COUNT] = {
	"Crossing",
	"Dribbling",
	"Finishing",
	"Heading",
	"LongShots",
	"Marking",
	"OffTheBall",
	"Passing",
	"PenaltyTaking",
	"Tackling",
	"Vision",
	"Handling",
	"AerialReach",
	"CommandOfArea",
	"Communication",
	"Kicking",
	"Throwing",
	"Anticipation",
	"Decisions",
	"OneOnOnes",
	"Positioning",
	"Reflexes",
	"FirstTouch",
	"Technique",
	"LeftFoot",
	"RightFoot",
	"Flair",
	"CornerTaking",
	"Teamwork",
	"WorkRate",
	"LongThrows",
	"Eccentricity",
	"RushingOut",
	"Punching",
	"Acceleration",
	"FreeKickTaking",
	"Strength",
	"Stamina",
	"Pace",
	"JumpingReach",
	"Leadership",
	"Dirtiness",
	"Balance",
	"Bravery",
	"Consistency",
	"Aggression",
	"Agility",
	"ImportantMatches",
	"InjuryProneness",
	"Versatility",
	"NaturalFitness",
	"Determination",
	"Composure",
	"Concentration",
	"Adaptability",
	"Ambition",
	"Loyalty",
	"Pressure",
	"Professionalism",
	"Sportsmanship",
	"Temperament",
	"Controversy",
};

#define ABILITY_CA 0
#define ABILITY_PA 2

/** Contract */
// 0xB8 bytes long
#define CONTRACTS_OFFSET_PERSON 0x08
#define CONTRACTS_OFFSET_TEAM 0x10
#define CONTRACTS_OFFSET_WEEKLY_WAGE 0x18
// 1 = player, 2 = coach, 3 = player/coach
#define CONTRACTS_OFFSET_JOB_TYPE 0x1C
#define CONTRACTS_OFFSET_LOYALTY_BONUS 0x30
#define CONTRACTS_OFFSET_START_DATE 0x3C
#define CONTRACTS_OFFSET_END_DATE 0x40
// 0 is part-time, 1 is full time, 2 is amateur, 3 is youth, 4 is non-contract
#define CONTRACTS_OFFSET_CONTRACT_TYPE 0xB4
// todo
// public const int CON_WEEKLY_WAGE = 0x20;  // u32 GBP p/w
// public const int CON_EXPIRY = 0x48;       // u32 FM-datum
// public const int CON_SQUAD_NUMBER = 0x5D; // byte
// public const int CON_STATUS_FLAGS = 0x57; // byte bitfield (transferstatus)
//   bit0 = Listed, bit3 = Listed by Request, bit4 = Not for Sale, bit5 = Set for Release

/** Continent */
// (+0x04 from here)
#define CONTINENT_OFFSET_NAME 0x18
#define CONTINENT_OFFSET_DEMONYM 0x28
#define CONTINENT_OFFSET_CONFED 0x30
#define CONTINENT_OFFSET_NAME_CODE 0x38

/** Nation */
// (+0x04 from here)
#define NATION_OFFSET_ROW_ID 0x08
#define NATION_OFFSET_NAME 0x18
#define NATION_OFFSET_NAME_SHORT 0x20
#define NATION_OFFSET_NAME_CODE 0x28
#define NATION_OFFSET_DEMONYM 0x30
#define NATION_OFFSET_CAPITAL_CITY 0xE8
#define NATION_OFFSET_CONTINENT 0xF0

/** Relationships */
#define RELATIONSHIP_OFFSET_TARGET_ADDRESS 0x00
#define RELATIONSHIP_OFFSET_TYPE 0x0A
#define RELATIONSHIP_OFFSET_INFO 0x0C

// Types:
//  - 00 47: Training happiness
//		- INFO byte 1 is value (0x00 to 0x64)
//  - 08 09: Other nationality
//		- Target address is pointer to Nationality object
//		- INFO is reason
//			- 64 00: No info
//			- 64 50: Eligible for nation
//			- 64 51: Has played for nation
//			- 64 52: Gained citizenship but treated as foreign
//			- 64 53: Declared for nation
//			- 64 54: Relative born in nation
//			- 64 55: Born in nation
//			- 64 56: Not eligible for nation
//			- 64 57: Gained citizenship through relative
//			- 64 58: Gained citizenship but not eligible for nation yet
//			- 64 59: Gained citizenship and declared for nation
//			- 64 5A: Gained citizenship through relative but not eligible for nation yet


/** Club */
#define CLUB_OFFSET_ROW_ID 0x08
#define CLUB_OFFSET_UNIQUE_ID 0x0C
#define CLUB_OFFSET_RANDOM_ID 0x10
#define CLUB_OFFSET_SQUAD_LIST_START 0x18
#define CLUB_OFFSET_SQUAD_LIST_END 0x20
#define CLUB_OFFSET_TEAM 0x30
#define CLUB_OFFSET_NAME 0xC0
#define CLUB_OFFSET_NAME_SHORT 0xC8
#define CLUB_OFFSET_NATION 0xD8

/** Team */
// 0xB0 size
#define TEAM_OFFSET_ROW_ID 0x08
#define TEAM_OFFSET_UNIQUE_ID 0x0C
#define TEAM_OFFSET_RANDOM_ID 0x10
// 0 for first team, 1 for reserves, 0x0C for U18, 0x16 for youth intake
#define TEAM_OFFSET_TEAM_TYPE 0x28
#define TEAM_OFFSET_TEAM_IS_NOT_SENIOR 0x29
#define TEAM_OFFSET_CLUB 0x30
#define TEAM_OFFSET_PLAYER_START 0x38
#define TEAM_OFFSET_PLAYER_END 0x40
#define TEAM_OFFSET_COMPETITION 0x50
#define TEAM_OFFSET_REPUTATION 0xA8

/** Competition */
// 0x1B0 size
#define COMPETITION_OFFSET_ROW_ID 0x08
#define COMPETITION_OFFSET_UNIQUE_ID 0x0C
#define COMPETITION_OFFSET_RANDOM_ID 0x10
// Longest name: The Emperor's Cup JFA All-Japan Soccer Championship Tournament, 62 chars
#define COMPETITION_OFFSET_LONG_NAME_ADDRESS 0x40
// (+0x04 from here)
// Longest name: Liga Profesional de Primera División Apertura, 46 chars
#define COMPETITION_OFFSET_SHORT_NAME_ADDRESS 0x48
#define COMPETITION_OFFSET_CONTINENT_ADDRESS 0x58
#define COMPETITION_OFFSET_NATION_ADDRESS 0x60

/** Continent */
#define CONTINENT_OFFSET_ROW_ID 0x08
#define CONTINENT_OFFSET_UNIQUE_ID 0x0C
#define CONTINENT_OFFSET_RANDOM_ID 0x10
#define CONTINENT_OFFSET_NAME_ADDRESS 0x18
// (+0x04 from here)
#define CONTINENT_OFFSET_DEMONYM_ADDRESS 0x28
#define CONTINENT_OFFSET_GOVERNING_BODY_NAME_ADDRESS 0x30

/** Nation */
#define NATION_OFFSET_ROW_ID 0x08
#define NATION_OFFSET_UNIQUE_ID 0x0C
#define NATION_OFFSET_RANDOM_ID 0x10
#define NATION_OFFSET_LONG_NAME_ADDRESS 0x18
// (+0x04 from here)
#define NATION_OFFSET_SHORT_NAME_ADDRESS 0x20
#define NATION_OFFSET_DEMONYM_ADDRESS 0x30
#define NATION_OFFSET_LANGUAGE_START 0x48
#define NATION_OFFSET_LANGUAGE_END 0x50

/** City */

// (+0x04 from here)
#define CITY_OFFSET_NAME 0x18

/** Injury */
// This is 0x60 bytes long
#define INJURY_OFFSET_ROW_ID 0x08
#define INJURY_OFFSET_UNIQUE_ID 0x0C
#define INJURY_OFFSET_RANDOM_ID 0x10
// (+0x04 from here)
#define INJURY_OFFSET_SHORT_NAME_ADDRESS 0x18
// (+0x04 from here)
#define INJURY_OFFSET_LONG_NAME_ADDRESS 0x20

#define STRING_OFFSET_LENGTH 0x0
#define STRING_OFFSET_VALUE 0x04
