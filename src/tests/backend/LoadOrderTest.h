/*  libloadorder

A library for reading and writing the load order of plugin files for
TES III: Morrowind, TES IV: Oblivion, TES V: Skyrim, Fallout 3 and
Fallout: New Vegas.

Copyright (C) 2015    WrinklyNinja

This file is part of libloadorder.

libloadorder is free software: you can redistribute
it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

libloadorder is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libloadorder.  If not, see
<http://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>

#include "libloadorder/constants.h"
#include "backend/GameSettings.h"
#include "backend/LoadOrder.h"
#include "backend/helpers.h"

namespace liblo {
    namespace test {
        class LoadOrderTest : public ::testing::TestWithParam<unsigned int> {
        protected:
            inline LoadOrderTest() :
                blankEsm("Blank.esm"),
                blankDifferentEsm("Blank - Different.esm"),
                blankMasterDependentEsm("Blank - Master Dependent.esm"),
                blankDifferentMasterDependentEsm("Blank - Different Master Dependent.esm"),
                blankEsp("Blank.esp"),
                blankDifferentEsp("Blank - Different.esp"),
                blankMasterDependentEsp("Blank - Master Dependent.esp"),
                blankDifferentMasterDependentEsp("Blank - Different Master Dependent.esp"),
                blankPluginDependentEsp("Blank - Plugin Dependent.esp"),
                blankDifferentPluginDependentEsp("Blank - Different Plugin Dependent.esp"),
                invalidPlugin("NotAPlugin.esm"),
                missingPlugin("missing.esm"),
                updateEsm("Update.esm"),
                nonAsciiEsm("Blàñk.esm"),
                gameSettings(GetParam(), getGamePath(GetParam()), getLocalPath(GetParam())) {}

            inline virtual void SetUp() {
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankEsm));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankDifferentEsm));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankMasterDependentEsm));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankDifferentMasterDependentEsm));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankEsp));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankDifferentEsp));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankMasterDependentEsp));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankDifferentMasterDependentEsp));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / blankDifferentPluginDependentEsp));
                ASSERT_FALSE(boost::filesystem::exists(gameSettings.getPluginsFolder() / missingPlugin));

                // Write out an non-empty, non-plugin file.
                boost::filesystem::ofstream out(gameSettings.getPluginsFolder() / invalidPlugin);
                out << "This isn't a valid plugin file.";
                out.close();
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / invalidPlugin));

                // Make sure the game master file exists.
                ASSERT_FALSE(boost::filesystem::exists(gameSettings.getPluginsFolder() / gameSettings.getMasterFile()));
                ASSERT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsm, gameSettings.getPluginsFolder() / gameSettings.getMasterFile()));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / gameSettings.getMasterFile()));

                // Make sure Update.esm exists.
                ASSERT_FALSE(boost::filesystem::exists(gameSettings.getPluginsFolder() / updateEsm));
                ASSERT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsm, gameSettings.getPluginsFolder() / updateEsm));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / updateEsm));

                // Make sure the non-ASCII plugin exists.
                ASSERT_FALSE(boost::filesystem::exists(gameSettings.getPluginsFolder() / nonAsciiEsm));
                ASSERT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsm, gameSettings.getPluginsFolder() / nonAsciiEsm));
                ASSERT_TRUE(boost::filesystem::exists(gameSettings.getPluginsFolder() / nonAsciiEsm));

                // Morrowind load order files have a slightly different
                // format and a prefix is necessary.
                std::string linePrefix = getActivePluginsFileLinePrefix(GetParam());

                // Write out an active plugins file, making it as invalid as
                // possible for the game to still fix.
                out.open(gameSettings.getActivePluginsFile());
                out << std::endl
                    << '#' << FromUTF8(blankDifferentEsm) << std::endl
                    << linePrefix << FromUTF8(blankEsm) << std::endl
                    << linePrefix << FromUTF8(blankEsp) << std::endl
                    << linePrefix << FromUTF8(nonAsciiEsm) << std::endl
                    << linePrefix << FromUTF8(blankEsm) << std::endl
                    << linePrefix << FromUTF8(invalidPlugin) << std::endl;
                out.close();

                if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                    // Write out the game's load order file, using the valid
                    // version of what's in the active plugins file, plus
                    // additional plugins.
                    out.open(gameSettings.getLoadOrderFile());
                    out << nonAsciiEsm << std::endl
                        << gameSettings.getMasterFile() << std::endl
                        << blankDifferentEsm << std::endl
                        << blankEsm << std::endl
                        << updateEsm << std::endl
                        << blankEsp << std::endl;
                    out.close();
                }
                else {
                    // Set load order using timestamps
                    std::vector<std::string> plugins({
                        gameSettings.getMasterFile(),
                        blankEsm,
                        blankDifferentEsm,
                        blankMasterDependentEsm,
                        blankDifferentMasterDependentEsm,
                        nonAsciiEsm,
                        blankEsp,  // Put a plugin before master to test fixup.
                        updateEsm,
                        blankDifferentEsp,
                        blankMasterDependentEsp,
                        blankDifferentMasterDependentEsp,
                        blankPluginDependentEsp,
                        blankDifferentPluginDependentEsp,
                    });
                    time_t modificationTime = time(NULL);  // Current time.
                    for (const auto &plugin : plugins) {
                        boost::filesystem::last_write_time(gameSettings.getPluginsFolder() / plugin, modificationTime);
                        modificationTime += 60;
                    }
                }
            }

            inline virtual void TearDown() {
                ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / invalidPlugin));
                ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / gameSettings.getMasterFile()));
                ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / updateEsm));
                ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / nonAsciiEsm));

                ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getActivePluginsFile()));
                if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                    ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getLoadOrderFile()));
            }

            inline std::string getGamePath(unsigned int gameId) const {
                if (gameId == LIBLO_GAME_TES3)
                    return "./Morrowind";
                else if (gameId == LIBLO_GAME_TES4)
                    return "./Oblivion";
                else
                    return "./Skyrim";
            }

            inline boost::filesystem::path getLocalPath(unsigned int gameId) const {
                if (gameId == LIBLO_GAME_TES3)
                    return "./local/Morrowind";
                else if (gameId == LIBLO_GAME_TES4)
                    return "./local/Oblivion";
                else
                    return "./local/Skyrim";
            }

            inline std::string getActivePluginsFileLinePrefix(unsigned int gameId) {
                if (gameId == LIBLO_GAME_TES3)
                    return "GameFile0=";
                else
                    return "";
            }

            GameSettings gameSettings;
            LoadOrder loadOrder;

            std::string blankEsm;
            std::string blankDifferentEsm;
            std::string blankMasterDependentEsm;
            std::string blankDifferentMasterDependentEsm;
            std::string blankEsp;
            std::string blankDifferentEsp;
            std::string blankMasterDependentEsp;
            std::string blankDifferentMasterDependentEsp;
            std::string blankPluginDependentEsp;
            std::string blankDifferentPluginDependentEsp;

            std::string invalidPlugin;
            std::string missingPlugin;
            std::string updateEsm;
            std::string nonAsciiEsm;
        };

        // Pass an empty first argument, as it's a prefix for the test instantation,
        // but we only have the one so no prefix is necessary.
        INSTANTIATE_TEST_CASE_P(,
                                LoadOrderTest,
                                ::testing::Values(
                                LIBLO_GAME_TES3,
                                LIBLO_GAME_TES4,
                                LIBLO_GAME_TES5,
                                LIBLO_GAME_FO3,
                                LIBLO_GAME_FNV));

        TEST_P(LoadOrderTest, settingAValidLoadOrderShouldNotThrow) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            EXPECT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithPluginsBeforeMastersShouldThrow) {
            std::vector<std::string> invalidLoadOrder({
                gameSettings.getMasterFile(),
                blankEsp,
                blankDifferentEsm,
            });
            EXPECT_ANY_THROW(loadOrder.setLoadOrder(invalidLoadOrder, gameSettings));
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithPluginsBeforeMastersShouldMakeNoChanges) {
            std::vector<std::string> invalidLoadOrder({
                gameSettings.getMasterFile(),
                blankEsp,
                blankDifferentEsm,
            });
            EXPECT_ANY_THROW(loadOrder.setLoadOrder(invalidLoadOrder, gameSettings));
            EXPECT_TRUE(loadOrder.getLoadOrder().empty());
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithAnInvalidPluginShouldThrow) {
            std::vector<std::string> invalidLoadOrder({
                gameSettings.getMasterFile(),
                invalidPlugin,
            });
            EXPECT_ANY_THROW(loadOrder.setLoadOrder(invalidLoadOrder, gameSettings));
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithAnInvalidPluginShouldMakeNoChanges) {
            std::vector<std::string> invalidLoadOrder({
                gameSettings.getMasterFile(),
                invalidPlugin,
            });
            EXPECT_ANY_THROW(loadOrder.setLoadOrder(invalidLoadOrder, gameSettings));
            EXPECT_TRUE(loadOrder.getLoadOrder().empty());
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithACaseInsensitiveDuplicatePluginShouldThrow) {
            std::vector<std::string> invalidLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                boost::to_lower_copy(blankEsm),
            });
            EXPECT_ANY_THROW(loadOrder.setLoadOrder(invalidLoadOrder, gameSettings));
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithACaseInsensitiveDuplicatePluginShouldMakeNoChanges) {
            std::vector<std::string> invalidLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                boost::to_lower_copy(blankEsm),
            });
            EXPECT_ANY_THROW(loadOrder.setLoadOrder(invalidLoadOrder, gameSettings));
            EXPECT_TRUE(loadOrder.getLoadOrder().empty());
        }

        TEST_P(LoadOrderTest, settingThenGettingLoadOrderShouldReturnTheSetLoadOrder) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_EQ(validLoadOrder, loadOrder.getLoadOrder());
        }

        TEST_P(LoadOrderTest, settingTheLoadOrderTwiceShouldReplaceTheFirstLoadOrder) {
            std::vector<std::string> firstLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            std::vector<std::string> secondLoadOrder({
                gameSettings.getMasterFile(),
                blankDifferentEsm,
                blankEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(firstLoadOrder, gameSettings));
            ASSERT_NO_THROW(loadOrder.setLoadOrder(secondLoadOrder, gameSettings));

            EXPECT_EQ(secondLoadOrder, loadOrder.getLoadOrder());
        }

        TEST_P(LoadOrderTest, settingAnInvalidLoadOrderShouldMakeNoChanges) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            std::vector<std::string> invalidLoadOrder({
                gameSettings.getMasterFile(),
                blankEsp,
                blankDifferentEsm,
            });

            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            EXPECT_ANY_THROW(loadOrder.setLoadOrder(invalidLoadOrder, gameSettings));

            EXPECT_EQ(validLoadOrder, loadOrder.getLoadOrder());
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithTheGameMasterNotAtTheBeginningShouldFailForTextfileLoadOrderGamesAndSucceedOtherwise) {
            std::vector<std::string> plugins({
                blankEsm,
                gameSettings.getMasterFile(),
            });
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_ANY_THROW(loadOrder.setLoadOrder(plugins, gameSettings));
            else
                EXPECT_NO_THROW(loadOrder.setLoadOrder(plugins, gameSettings));
        }

        TEST_P(LoadOrderTest, settingALoadOrderWithTheGameMasterNotAtTheBeginningShouldMakeNoChangesForTextfileLoadOrderGames) {
            std::vector<std::string> plugins({
                blankEsm,
                gameSettings.getMasterFile(),
            });
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                EXPECT_ANY_THROW(loadOrder.setLoadOrder(plugins, gameSettings));
                EXPECT_TRUE(loadOrder.getLoadOrder().empty());
            }
        }

        TEST_P(LoadOrderTest, positionOfAMissingPluginShouldEqualTheLoadOrderSize) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_EQ(validLoadOrder.size(), loadOrder.getPosition(missingPlugin));
        }

        TEST_P(LoadOrderTest, positionOfAPluginShouldBeEqualToItsLoadOrderIndex) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_EQ(1, loadOrder.getPosition(blankEsm));
        }

        TEST_P(LoadOrderTest, gettingAPluginsPositionShouldBeCaseInsensitive) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_EQ(1, loadOrder.getPosition(boost::to_lower_copy(blankEsm)));
        }

        TEST_P(LoadOrderTest, gettingPluginAtAPositionGreaterThanTheHighestIndexShouldThrow) {
            EXPECT_ANY_THROW(loadOrder.getPluginAtPosition(0));
        }

        TEST_P(LoadOrderTest, gettingPluginAtAValidPositionShouldReturnItsLoadOrderIndex) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_EQ(blankEsm, loadOrder.getPluginAtPosition(1));
        }

        TEST_P(LoadOrderTest, settingAPluginThatIsNotTheGameMasterFileToLoadFirstShouldThrowForTextfileLoadOrderGamesAndNotOtherwise) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_ANY_THROW(loadOrder.setPosition(blankEsm, 0, gameSettings));
            else {
                EXPECT_NO_THROW(loadOrder.setPosition(blankEsm, 0, gameSettings));
            }
        }

        TEST_P(LoadOrderTest, settingAPluginThatIsNotTheGameMasterFileToLoadFirstForATextfileBasedGameShouldMakeNoChanges) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                EXPECT_ANY_THROW(loadOrder.setPosition(blankEsm, 0, gameSettings));
                EXPECT_TRUE(loadOrder.getLoadOrder().empty());
            }
        }

        TEST_P(LoadOrderTest, settingAPluginThatIsNotTheGameMasterFileToLoadFirstForATimestampBasedGameShouldSucceed) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TIMESTAMP) {
                EXPECT_NO_THROW(loadOrder.setPosition(blankEsm, 0, gameSettings));
                EXPECT_FALSE(loadOrder.getLoadOrder().empty());
                EXPECT_EQ(0, loadOrder.getPosition(blankEsm));
            }
        }

        TEST_P(LoadOrderTest, settingTheGameMasterFileToLoadAfterAnotherPluginShouldThrowForTextfileLoadOrderGamesAndNotOtherwise) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_ANY_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 1, gameSettings));
            else
                EXPECT_NO_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 1, gameSettings));
        }

        TEST_P(LoadOrderTest, settingTheGameMasterFileToLoadAfterAnotherPluginShouldMakeNoChangesForTextfileLoadOrderGames) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                EXPECT_ANY_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 1, gameSettings));
                EXPECT_EQ(0, loadOrder.getPosition(gameSettings.getMasterFile()));
            }
        }

        TEST_P(LoadOrderTest, settingTheGameMasterFileToLoadAfterAnotherPluginForATextfileBasedGameShouldMakeNoChanges) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                ASSERT_ANY_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 1, gameSettings));
                EXPECT_EQ(blankEsm, loadOrder.getPluginAtPosition(1));
            }
        }

        TEST_P(LoadOrderTest, settingTheGameMasterFileToLoadAfterAnotherPluginForATimestampBasedGameShouldSucceed) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TIMESTAMP) {
                ASSERT_NO_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 1, gameSettings));
                EXPECT_EQ(blankEsm, loadOrder.getPluginAtPosition(0));
                EXPECT_EQ(gameSettings.getMasterFile(), loadOrder.getPluginAtPosition(1));
            }
        }

        TEST_P(LoadOrderTest, settingThePositionOfAnInvalidPluginShouldThrow) {
            ASSERT_NO_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 0, gameSettings));

            EXPECT_ANY_THROW(loadOrder.setPosition(invalidPlugin, 1, gameSettings));
        }

        TEST_P(LoadOrderTest, settingThePositionOfAnInvalidPluginShouldMakeNoChanges) {
            ASSERT_NO_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 0, gameSettings));

            ASSERT_ANY_THROW(loadOrder.setPosition(invalidPlugin, 1, gameSettings));
            EXPECT_EQ(1, loadOrder.getLoadOrder().size());
        }

        TEST_P(LoadOrderTest, settingThePositionOfAPluginToGreaterThanTheLoadOrderSizeShouldPutThePluginAtTheEnd) {
            ASSERT_NO_THROW(loadOrder.setPosition(gameSettings.getMasterFile(), 0, gameSettings));

            EXPECT_NO_THROW(loadOrder.setPosition(blankEsm, 2, gameSettings));
            EXPECT_EQ(2, loadOrder.getLoadOrder().size());
            EXPECT_EQ(1, loadOrder.getPosition(blankEsm));
        }

        TEST_P(LoadOrderTest, settingThePositionOfAPluginShouldBeCaseInsensitive) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_NO_THROW(loadOrder.setPosition(boost::to_lower_copy(blankEsm), 2, gameSettings));

            std::vector<std::string> expectedLoadOrder({
                gameSettings.getMasterFile(),
                blankDifferentEsm,
                blankEsm,
            });
            EXPECT_EQ(expectedLoadOrder, loadOrder.getLoadOrder());
        }

        TEST_P(LoadOrderTest, settingANonMasterPluginToLoadBeforeAMasterPluginShouldThrow) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_ANY_THROW(loadOrder.setPosition(blankEsp, 1, gameSettings));
        }

        TEST_P(LoadOrderTest, settingANonMasterPluginToLoadBeforeAMasterPluginShouldMakeNoChanges) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_ANY_THROW(loadOrder.setPosition(blankEsp, 1, gameSettings));
            EXPECT_EQ(validLoadOrder, loadOrder.getLoadOrder());
        }

        TEST_P(LoadOrderTest, settingAMasterToLoadAfterAPluginShouldThrow) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_ANY_THROW(loadOrder.setPosition(blankEsm, 2, gameSettings));
        }

        TEST_P(LoadOrderTest, settingAMasterToLoadAfterAPluginShouldMakeNoChanges) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_ANY_THROW(loadOrder.setPosition(blankEsm, 2, gameSettings));
            EXPECT_EQ(validLoadOrder, loadOrder.getLoadOrder());
        }

        TEST_P(LoadOrderTest, clearingLoadOrderShouldRemoveAllPluginsFromTheLoadOrder) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_NO_THROW(loadOrder.clear());
            EXPECT_TRUE(loadOrder.getLoadOrder().empty());
        }

        TEST_P(LoadOrderTest, checkingIfAnInactivePluginIsActiveShouldReturnFalse) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_FALSE(loadOrder.isActive(blankEsm));
        }

        TEST_P(LoadOrderTest, checkingIfAPluginNotInTheLoadOrderIsActiveShouldReturnFalse) {
            EXPECT_FALSE(loadOrder.isActive(blankEsp));
        }

        TEST_P(LoadOrderTest, activatingAnInvalidPluginShouldThrow) {
            EXPECT_ANY_THROW(loadOrder.activate(invalidPlugin, gameSettings));
        }

        TEST_P(LoadOrderTest, activatingANonMasterPluginNotInTheLoadOrderShouldAppendItToTheLoadOrder) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            ASSERT_EQ(3, loadOrder.getLoadOrder().size());

            EXPECT_NO_THROW(loadOrder.activate(blankEsp, gameSettings));
            EXPECT_EQ(3, loadOrder.getPosition(blankEsp));
            EXPECT_TRUE(loadOrder.isActive(blankEsp));
        }

        TEST_P(LoadOrderTest, activatingAMasterPluginNotInTheLoadOrderShouldInsertItAfterAllOtherMasters) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            ASSERT_EQ(3, loadOrder.getLoadOrder().size());

            EXPECT_NO_THROW(loadOrder.activate(blankDifferentEsm, gameSettings));
            ASSERT_EQ(4, loadOrder.getLoadOrder().size());
            EXPECT_EQ(2, loadOrder.getPosition(blankDifferentEsm));
            EXPECT_TRUE(loadOrder.isActive(blankDifferentEsm));
        }

        TEST_P(LoadOrderTest, activatingTheGameMasterFileNotInTheLoadOrderShouldInsertItAtTheBeginningForTextfileBasedGamesAndAfterAllOtherMastersOtherwise) {
            ASSERT_NO_THROW(loadOrder.activate(blankEsm, gameSettings));

            EXPECT_NO_THROW(loadOrder.activate(gameSettings.getMasterFile(), gameSettings));
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_EQ(0, loadOrder.getPosition(gameSettings.getMasterFile()));
            else
                EXPECT_EQ(1, loadOrder.getPosition(gameSettings.getMasterFile()));
        }

        TEST_P(LoadOrderTest, activatingAPluginInTheLoadOrderShouldSetItToActive) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            ASSERT_FALSE(loadOrder.isActive(blankDifferentEsm));

            EXPECT_NO_THROW(loadOrder.activate(blankDifferentEsm, gameSettings));
            EXPECT_TRUE(loadOrder.isActive(blankDifferentEsm));
        }

        TEST_P(LoadOrderTest, checkingIfAPluginIsActiveShouldBeCaseInsensitive) {
            EXPECT_NO_THROW(loadOrder.activate(blankEsm, gameSettings));
            EXPECT_TRUE(loadOrder.isActive(boost::to_lower_copy(blankEsm)));
        }

        TEST_P(LoadOrderTest, activatingAPluginShouldBeCaseInsensitive) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            EXPECT_NO_THROW(loadOrder.activate(boost::to_lower_copy(blankEsm), gameSettings));

            EXPECT_TRUE(loadOrder.isActive(blankEsm));
            EXPECT_EQ(validLoadOrder, loadOrder.getLoadOrder());
        }

        TEST_P(LoadOrderTest, activatingAPluginWhenMaxNumberAreAlreadyActiveShouldThrow) {
            // Create plugins to test active plugins limit with. Do it
            // here because it's too expensive to do for every test.
            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i) {
                EXPECT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsp, gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
                EXPECT_NO_THROW(loadOrder.activate(std::to_string(i) + ".esp", gameSettings));
            }

            EXPECT_ANY_THROW(loadOrder.activate(blankEsm, gameSettings));

            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i)
                EXPECT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
        }

        TEST_P(LoadOrderTest, activatingAPluginWhenMaxNumberAreAlreadyActiveShouldMakeNoChanges) {
            // Create plugins to test active plugins limit with. Do it
            // here because it's too expensive to do for every test.
            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i) {
                EXPECT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsp, gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
                EXPECT_NO_THROW(loadOrder.activate(std::to_string(i) + ".esp", gameSettings));
            }

            EXPECT_ANY_THROW(loadOrder.activate(blankEsm, gameSettings));
            EXPECT_FALSE(loadOrder.isActive(blankEsm));

            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i)
                EXPECT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
        }

        TEST_P(LoadOrderTest, deactivatingAPluginNotInTheLoadOrderShouldDoNothing) {
            EXPECT_NO_THROW(loadOrder.deactivate(blankEsp, gameSettings));
            EXPECT_FALSE(loadOrder.isActive(blankEsp));
            EXPECT_TRUE(loadOrder.getLoadOrder().empty());
        }

        TEST_P(LoadOrderTest, deactivatingTheGameMasterFileShouldThrowForTextfileLoadOrderGamesAndNotOtherwise) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_ANY_THROW(loadOrder.deactivate(gameSettings.getMasterFile(), gameSettings));
            else
                EXPECT_NO_THROW(loadOrder.deactivate(gameSettings.getMasterFile(), gameSettings));
        }

        TEST_P(LoadOrderTest, deactivatingTheGameMasterFileShouldForTextfileLoadOrderGamesShouldMakeNoChanges) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                EXPECT_ANY_THROW(loadOrder.deactivate(gameSettings.getMasterFile(), gameSettings));
                EXPECT_FALSE(loadOrder.isActive(gameSettings.getMasterFile()));
            }
        }

        TEST_P(LoadOrderTest, forSkyrimDeactivatingUpdateEsmShouldThrow) {
            if (gameSettings.getId() == LIBLO_GAME_TES5)
                EXPECT_ANY_THROW(loadOrder.deactivate(updateEsm, gameSettings));
        }

        TEST_P(LoadOrderTest, forSkyrimDeactivatingUpdateEsmShouldMakeNoChanges) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                updateEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            ASSERT_NO_THROW(loadOrder.activate(updateEsm, gameSettings));

            if (gameSettings.getId() == LIBLO_GAME_TES5) {
                EXPECT_ANY_THROW(loadOrder.deactivate(updateEsm, gameSettings));
                EXPECT_TRUE(loadOrder.isActive(updateEsm));
            }
        }

        TEST_P(LoadOrderTest, deactivatingAnInactivePluginShouldHaveNoEffect) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            ASSERT_FALSE(loadOrder.isActive(blankEsm));

            EXPECT_NO_THROW(loadOrder.deactivate(blankEsm, gameSettings));
            EXPECT_FALSE(loadOrder.isActive(blankEsm));
        }

        TEST_P(LoadOrderTest, deactivatingAnActivePluginShouldMakeItInactive) {
            ASSERT_NO_THROW(loadOrder.activate(blankEsp, gameSettings));
            ASSERT_TRUE(loadOrder.isActive(blankEsp));

            EXPECT_NO_THROW(loadOrder.deactivate(blankEsp, gameSettings));
            EXPECT_FALSE(loadOrder.isActive(blankEsp));
        }

        TEST_P(LoadOrderTest, settingThePositionOfAnActivePluginShouldKeepItActive) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            ASSERT_NO_THROW(loadOrder.activate(blankEsm, gameSettings));

            loadOrder.setPosition(blankEsm, 2, gameSettings);
            EXPECT_TRUE(loadOrder.isActive(blankEsm));
        }

        TEST_P(LoadOrderTest, settingThePositionOfAnInactivePluginShouldKeepItInactive) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));

            loadOrder.setPosition(blankEsm, 2, gameSettings);
            EXPECT_FALSE(loadOrder.isActive(blankEsm));
        }

        TEST_P(LoadOrderTest, settingLoadOrderShouldActivateTheGameMasterForTextfileBasedGamesAndNotOtherwise) {
            std::vector<std::string> firstLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(firstLoadOrder, gameSettings));
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_TRUE(loadOrder.isActive(gameSettings.getMasterFile()));
            else
                EXPECT_FALSE(loadOrder.isActive(gameSettings.getMasterFile()));
        }

        TEST_P(LoadOrderTest, settingANewLoadOrderShouldRetainTheActiveStateOfPluginsInTheOldLoadOrder) {
            std::vector<std::string> firstLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(firstLoadOrder, gameSettings));
            ASSERT_NO_THROW(loadOrder.activate(blankEsm, gameSettings));

            std::vector<std::string> secondLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(secondLoadOrder, gameSettings));

            EXPECT_TRUE(loadOrder.isActive(blankEsm));
            EXPECT_FALSE(loadOrder.isActive(blankEsp));
        }

        TEST_P(LoadOrderTest, settingInvalidActivePluginsShouldThrow) {
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
                invalidPlugin,
            });
            EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
        }

        TEST_P(LoadOrderTest, settingInvalidActivePluginsShouldMakeNoChanges) {
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
                invalidPlugin,
            });
            EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
            EXPECT_TRUE(loadOrder.getActivePlugins().empty());
        }

        TEST_P(LoadOrderTest, settingMoreThanMaxNumberActivePluginsShouldThrow) {
            // Create plugins to test active plugins limit with. Do it
            // here because it's too expensive to do for every test.
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
            });
            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i) {
                EXPECT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsp, gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
                activePlugins.insert(std::to_string(i) + ".esp");
            }

            EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));

            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i)
                EXPECT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
        }

        TEST_P(LoadOrderTest, settingMoreThanMaxNumberActivePluginsShouldMakeNoChanges) {
            // Create plugins to test active plugins limit with. Do it
            // here because it's too expensive to do for every test.
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
            });
            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i) {
                EXPECT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsp, gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
                activePlugins.insert(std::to_string(i) + ".esp");
            }

            EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
            EXPECT_TRUE(loadOrder.getActivePlugins().empty());

            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i)
                EXPECT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
        }

        TEST_P(LoadOrderTest, settingActivePluginsWithoutGameMasterShouldThrowForTextfileBasedGamesAndNotOtherwise) {
            std::unordered_set<std::string> activePlugins({
                updateEsm,
                blankEsm,
            });
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
            else
                EXPECT_NO_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
        }

        TEST_P(LoadOrderTest, settingActivePluginsWithoutGameMasterShouldMakeNoChangesForTextfileBasedGames) {
            std::unordered_set<std::string> activePlugins({
                updateEsm,
                blankEsm,
            });
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
                EXPECT_TRUE(loadOrder.getActivePlugins().empty());
            }
        }

        TEST_P(LoadOrderTest, settingActivePluginsWithoutUpdateEsmWhenItExistsShouldThrowForSkyrimAndNotOtherwise) {
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                blankEsm,
            });
            if (gameSettings.getId() == LIBLO_GAME_TES5)
                EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
            else
                EXPECT_NO_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
        }

        TEST_P(LoadOrderTest, settingActivePluginsWithoutUpdateEsmWhenItExistsShouldMakeNoChangesForSkyrim) {
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                blankEsm,
            });
            if (gameSettings.getId() == LIBLO_GAME_TES5) {
                EXPECT_ANY_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
                EXPECT_TRUE(loadOrder.getActivePlugins().empty());
            }
        }

        TEST_P(LoadOrderTest, settingActivePluginsWithoutUpdateEsmWhenItDoesNotExistShouldNotThrow) {
            ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / updateEsm));

            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                blankEsm,
            });
            EXPECT_NO_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));
        }

        TEST_P(LoadOrderTest, settingActivePluginsShouldDeactivateAnyOthersInLoadOrderCaseInsensitively) {
            std::vector<std::string> validLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankEsp,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(validLoadOrder, gameSettings));
            ASSERT_NO_THROW(loadOrder.activate(blankEsp, gameSettings));

            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
                boost::to_lower_copy(blankEsm),
            });
            EXPECT_NO_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));

            std::unordered_set<std::string> expectedActivePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
                blankEsm,
            });
            EXPECT_EQ(expectedActivePlugins, loadOrder.getActivePlugins());
        }

        TEST_P(LoadOrderTest, settingActivePluginsNotInLoadOrderShouldAddThem) {
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
                blankEsm,
            });
            std::vector<std::string> expectedLoadOrder({
                gameSettings.getMasterFile(),
                updateEsm,
                blankEsm,
            });
            ASSERT_TRUE(loadOrder.getLoadOrder().empty());

            EXPECT_NO_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));

            std::vector<std::string> newLoadOrder(loadOrder.getLoadOrder());
            EXPECT_EQ(3, newLoadOrder.size());
            EXPECT_EQ(1, count(std::begin(newLoadOrder), std::end(newLoadOrder), gameSettings.getMasterFile()));
            EXPECT_EQ(1, count(std::begin(newLoadOrder), std::end(newLoadOrder), updateEsm));
            EXPECT_EQ(1, count(std::begin(newLoadOrder), std::end(newLoadOrder), blankEsm));
        }

        TEST_P(LoadOrderTest, isSynchronisedForTimestampBasedGames) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TIMESTAMP)
                EXPECT_TRUE(LoadOrder::isSynchronised(gameSettings));
        }

        TEST_P(LoadOrderTest, isSynchronisedForTextfileBasedGamesIfLoadOrderFileDoesNotExist) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TIMESTAMP)
                return;

            ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getLoadOrderFile()));

            EXPECT_TRUE(LoadOrder::isSynchronised(gameSettings));
        }

        TEST_P(LoadOrderTest, isSynchronisedForTextfileBasedGamesIfActivePluginsFileDoesNotExist) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TIMESTAMP)
                return;

            ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getActivePluginsFile()));

            EXPECT_TRUE(LoadOrder::isSynchronised(gameSettings));
        }

        TEST_P(LoadOrderTest, isSynchronisedForTextfileBasedGamesWhenLoadOrderAndActivePluginsFileContentsAreEquivalent) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TIMESTAMP)
                return;

            EXPECT_TRUE(LoadOrder::isSynchronised(gameSettings));
        }

        TEST_P(LoadOrderTest, isNotSynchronisedForTextfileBasedGamesWhenLoadOrderAndActivePluginsFileContentsAreNotEquivalent) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TIMESTAMP)
                return;

            boost::filesystem::ofstream out(gameSettings.getLoadOrderFile(), std::ios_base::app);
            out << blankEsm << std::endl;

            EXPECT_FALSE(LoadOrder::isSynchronised(gameSettings));
        }

        TEST_P(LoadOrderTest, loadingDataShouldNotThrowIfActivePluginsFileDoesNotExist) {
            ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getActivePluginsFile()));

            EXPECT_NO_THROW(loadOrder.load(gameSettings));
        }

        TEST_P(LoadOrderTest, loadingDataShouldActivateNoPluginsIfActivePluginsFileDoesNotExist) {
            ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getActivePluginsFile()));

            ASSERT_NO_THROW(loadOrder.load(gameSettings));

            EXPECT_TRUE(loadOrder.getActivePlugins().empty());
        }

        TEST_P(LoadOrderTest, loadingDataShouldActivateTheGameMasterForTextfileBasedGamesAndNotOtherwise) {
            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            int count = loadOrder.getActivePlugins().count(gameSettings.getMasterFile());
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                EXPECT_EQ(1, count);
            else
                EXPECT_EQ(0, count);
        }

        TEST_P(LoadOrderTest, loadingDataShouldActivateUpdateEsmWhenItExistsForSkyrimAndNotOtherwise) {
            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            int count = loadOrder.getActivePlugins().count(updateEsm);
            if (gameSettings.getId() == LIBLO_GAME_TES5)
                EXPECT_EQ(1, count);
            else
                EXPECT_EQ(0, count);
        }

        TEST_P(LoadOrderTest, loadingDataShouldNotActivateUpdateEsmWhenItDoesNotExist) {
            ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / updateEsm));

            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            EXPECT_EQ(0, loadOrder.getActivePlugins().count(updateEsm));
        }

        TEST_P(LoadOrderTest, loadingDataWithMoreThanMaxNumberActivePluginsShouldStopWhenMaxIsReached) {
            // Create plugins to test active plugins limit with. Do it
            // here because it's too expensive to do for every test.
            std::unordered_set<std::string> expectedActivePlugins;

            std::string linePrefix = getActivePluginsFileLinePrefix(gameSettings.getId());
            boost::filesystem::ofstream out(gameSettings.getActivePluginsFile());

            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                out << linePrefix << FromUTF8(gameSettings.getMasterFile()) << std::endl;
                expectedActivePlugins.insert(gameSettings.getMasterFile());

                if (gameSettings.getId() == LIBLO_GAME_TES5) {
                    out << linePrefix << FromUTF8(updateEsm) << std::endl;
                    expectedActivePlugins.insert(updateEsm);
                }
            }

            for (size_t i = 0; i < LoadOrder::maxActivePlugins - expectedActivePlugins.size(); ++i) {
                std::string filename = std::to_string(i) + ".esp";
                EXPECT_NO_THROW(boost::filesystem::copy_file(gameSettings.getPluginsFolder() / blankEsp, gameSettings.getPluginsFolder() / filename));
                out << linePrefix << filename << std::endl;
                expectedActivePlugins.insert(filename);
            }
            out.close();

            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            EXPECT_EQ(expectedActivePlugins.size(), loadOrder.getActivePlugins().size());
            EXPECT_EQ(expectedActivePlugins, loadOrder.getActivePlugins());

            for (size_t i = 0; i < LoadOrder::maxActivePlugins; ++i)
                EXPECT_NO_THROW(boost::filesystem::remove(gameSettings.getPluginsFolder() / (std::to_string(i) + ".esp")));
        }

        TEST_P(LoadOrderTest, loadingDataShouldFixInvalidDataWhenReadingActivePluginsFile) {
            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            std::unordered_set<std::string> expectedActivePlugins({
                nonAsciiEsm,
                blankEsm,
                blankEsp,
            });
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                expectedActivePlugins.insert(gameSettings.getMasterFile());

                if (gameSettings.getId() == LIBLO_GAME_TES5)
                    expectedActivePlugins.insert(updateEsm);
            }
            EXPECT_EQ(expectedActivePlugins, loadOrder.getActivePlugins());
        }

        TEST_P(LoadOrderTest, loadingDataShouldPreferLoadOrderFileForTextfileBasedGamesOtherwiseUseTimestamps) {
            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            std::vector<std::string> expectedLoadOrder;
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                expectedLoadOrder = std::vector<std::string>({
                    gameSettings.getMasterFile(),
                    nonAsciiEsm,
                    blankDifferentEsm,
                    blankEsm,
                    updateEsm,
                });
                EXPECT_TRUE(equal(begin(expectedLoadOrder), end(expectedLoadOrder), begin(loadOrder.getLoadOrder())));
            }
            else {
                expectedLoadOrder = std::vector<std::string>({
                    gameSettings.getMasterFile(),
                    blankEsm,
                    blankDifferentEsm,
                    blankMasterDependentEsm,
                    blankDifferentMasterDependentEsm,
                    nonAsciiEsm,
                    updateEsm,
                    blankEsp,
                    blankDifferentEsp,
                    blankMasterDependentEsp,
                    blankDifferentMasterDependentEsp,
                    blankPluginDependentEsp,
                    blankDifferentPluginDependentEsp,
                });
                EXPECT_EQ(expectedLoadOrder, loadOrder.getLoadOrder());
            }
        }

        TEST_P(LoadOrderTest, loadingDataShouldFallBackToActivePluginsFileForTextfileBasedGamesOtherwiseUseTimestamps) {
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE)
                ASSERT_NO_THROW(boost::filesystem::remove(gameSettings.getLoadOrderFile()));

            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            std::vector<std::string> expectedLoadOrder;
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                expectedLoadOrder = std::vector<std::string>({
                    gameSettings.getMasterFile(),
                    nonAsciiEsm,
                    blankEsm,
                    updateEsm,
                });
                EXPECT_TRUE(equal(begin(expectedLoadOrder), end(expectedLoadOrder), begin(loadOrder.getLoadOrder())));
            }
            else {
                expectedLoadOrder = std::vector<std::string>({
                    gameSettings.getMasterFile(),
                    blankEsm,
                    blankDifferentEsm,
                    blankMasterDependentEsm,
                    blankDifferentMasterDependentEsm,
                    nonAsciiEsm,
                    updateEsm,
                    blankEsp,
                    blankDifferentEsp,
                    blankMasterDependentEsp,
                    blankDifferentMasterDependentEsp,
                    blankPluginDependentEsp,
                    blankDifferentPluginDependentEsp,
                });
                EXPECT_EQ(expectedLoadOrder, loadOrder.getLoadOrder());
            }
        }

        TEST_P(LoadOrderTest, loadingDataTwiceShouldDiscardTheDataRead) {
            ASSERT_NO_THROW(loadOrder.load(gameSettings));

            std::string linePrefix = getActivePluginsFileLinePrefix(gameSettings.getId());
            boost::filesystem::ofstream out(gameSettings.getActivePluginsFile(), std::ios_base::trunc);
            out << linePrefix << FromUTF8(blankEsp) << std::endl;
            out.close();

            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                out.open(gameSettings.getLoadOrderFile(), std::ios_base::trunc);
                out << blankDifferentEsm << std::endl;
                out.close();
            }

            EXPECT_NO_THROW(loadOrder.load(gameSettings));

            std::unordered_set<std::string> expectedActivePlugins({
                blankEsp,
            });
            std::vector<std::string> expectedLoadOrder({
                gameSettings.getMasterFile(),
                blankEsm,
                blankDifferentEsm,
                blankMasterDependentEsm,
                blankDifferentMasterDependentEsm,
                nonAsciiEsm,
                updateEsm,
                blankEsp,
                blankDifferentEsp,
                blankMasterDependentEsp,
                blankDifferentMasterDependentEsp,
                blankPluginDependentEsp,
                blankDifferentPluginDependentEsp,
            });
            if (gameSettings.getLoadOrderMethod() == LIBLO_METHOD_TEXTFILE) {
                EXPECT_NE(expectedLoadOrder, loadOrder.getLoadOrder());
                EXPECT_TRUE(is_permutation(begin(expectedLoadOrder), end(expectedLoadOrder), begin(loadOrder.getLoadOrder())));

                expectedActivePlugins.insert(gameSettings.getMasterFile());

                if (gameSettings.getId() == LIBLO_GAME_TES5)
                    expectedActivePlugins.insert(updateEsm);
            }
            else {
                EXPECT_EQ(expectedLoadOrder, loadOrder.getLoadOrder());
            }

            EXPECT_EQ(expectedActivePlugins, loadOrder.getActivePlugins());
        }

        TEST_P(LoadOrderTest, savingShouldSetTimestampsForTimestampBasedGamesAndWriteToLoadOrderAndActivePluginsFilesOtherwise) {
            std::vector<std::string> plugins({
                gameSettings.getMasterFile(),
                blankEsm,
                blankMasterDependentEsm,
                blankDifferentEsm,
                blankDifferentMasterDependentEsm,
            });
            ASSERT_NO_THROW(loadOrder.setLoadOrder(plugins, gameSettings));

            EXPECT_NO_THROW(loadOrder.save(gameSettings));

            ASSERT_NO_THROW(loadOrder.load(gameSettings));

            EXPECT_TRUE(equal(begin(plugins), end(plugins), begin(loadOrder.getLoadOrder())));
        }

        TEST_P(LoadOrderTest, savingShouldWriteActivePluginsToActivePluginsFile) {
            std::unordered_set<std::string> activePlugins({
                gameSettings.getMasterFile(),
                updateEsm,
                blankEsm,
            });
            ASSERT_NO_THROW(loadOrder.setActivePlugins(activePlugins, gameSettings));

            EXPECT_NO_THROW(loadOrder.save(gameSettings));

            ASSERT_NO_THROW(loadOrder.load(gameSettings));

            EXPECT_EQ(activePlugins, loadOrder.getActivePlugins());
        }
    }
}
