/*
 * This file is part of libloadorder
 *
 * Copyright (C) 2017 Oliver Hamlet
 *
 * libloadorder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libloadorder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libloadorder. If not, see <http://www.gnu.org/licenses/>.
 */

use std::borrow::Cow;
use std::convert::From;
use std::error;
use std::fmt;
use std::io;
use std::path::PathBuf;
use std::string::FromUtf8Error;
use std::time;

use esplugin;
use esplugin::GameId as EspmId;
use regex;

#[cfg(windows)]
use app_dirs;

#[derive(Clone, Copy, PartialEq, Eq, Debug, Hash)]
pub enum LoadOrderMethod {
    Timestamp,
    Textfile,
    Asterisk,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug, Hash)]
pub enum GameId {
    Morrowind = 1,
    Oblivion,
    Skyrim,
    Fallout3,
    FalloutNV,
    Fallout4,
    SkyrimSE,
    Fallout4VR,
    SkyrimVR,
}

impl GameId {
    pub fn to_esplugin_id(self) -> EspmId {
        match self {
            GameId::Morrowind => EspmId::Morrowind,
            GameId::Oblivion => EspmId::Oblivion,
            GameId::Skyrim => EspmId::Skyrim,
            GameId::SkyrimSE => EspmId::SkyrimSE,
            GameId::SkyrimVR => EspmId::SkyrimSE,
            GameId::Fallout3 => EspmId::Fallout3,
            GameId::FalloutNV => EspmId::FalloutNV,
            GameId::Fallout4 => EspmId::Fallout4,
            GameId::Fallout4VR => EspmId::Fallout4,
        }
    }

    pub fn supports_light_masters(self) -> bool {
        use enums::GameId::*;
        match self {
            Fallout4 | Fallout4VR | SkyrimSE | SkyrimVR => true,
            _ => false,
        }
    }
}

#[derive(Debug)]
pub enum Error {
    InvalidPath(PathBuf),
    IoError(io::Error),
    NoFilename,
    SystemTimeError(time::SystemTimeError),
    NotUtf8(Vec<u8>),
    DecodeError(Cow<'static, str>),
    EncodeError(Cow<'static, str>),
    PluginParsingError,
    PluginNotFound(String),
    TooManyActivePlugins,
    InvalidRegex,
    DuplicatePlugin,
    NonMasterBeforeMaster,
    GameMasterMustLoadFirst,
    InvalidPlugin(String),
    ImplicitlyActivePlugin(String),
    NoLocalAppData,
    /// First string is the plugin, second is the master.
    UnrepresentedHoist(String, String),
    InstalledPlugin(String),
}

#[cfg(windows)]
impl From<app_dirs::AppDirsError> for Error {
    fn from(error: app_dirs::AppDirsError) -> Self {
        match error {
            app_dirs::AppDirsError::Io(x) => Error::IoError(x),
            _ => Error::NoLocalAppData,
        }
    }
}

impl From<io::Error> for Error {
    fn from(error: io::Error) -> Self {
        Error::IoError(error)
    }
}

impl From<time::SystemTimeError> for Error {
    fn from(error: time::SystemTimeError) -> Self {
        Error::SystemTimeError(error)
    }
}

impl From<regex::Error> for Error {
    fn from(_: regex::Error) -> Self {
        Error::InvalidRegex
    }
}

impl From<FromUtf8Error> for Error {
    fn from(error: FromUtf8Error) -> Self {
        Error::NotUtf8(error.into_bytes())
    }
}

impl From<esplugin::Error> for Error {
    fn from(error: esplugin::Error) -> Self {
        match error {
            esplugin::Error::IoError(x) => Error::IoError(x),
            esplugin::Error::NoFilename => Error::NoFilename,
            esplugin::Error::ParsingIncomplete | esplugin::Error::ParsingError => {
                Error::PluginParsingError
            }
            esplugin::Error::DecodeError(x) => Error::DecodeError(x),
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Error::InvalidPath(ref x) => write!(f, "The path \"{:?}\" is invalid", x),
            Error::IoError(ref x) => x.fmt(f),
            Error::NoFilename => write!(f, "The plugin path has no filename part"),
            Error::SystemTimeError(ref x) => x.fmt(f),
            Error::NotUtf8(ref x) => write!(f, "Expected a UTF-8 string, got bytes {:?}", x),
            Error::DecodeError(_) => write!(f, "Text could not be decoded from Windows-1252"),
            Error::EncodeError(_) => write!(f, "Text could not be encoded in Windows-1252"),
            Error::PluginParsingError => {
                write!(f, "An error was encountered while parsing a plugin")
            }
            Error::PluginNotFound(ref x) => {
                write!(f, "The plugin \"{}\" is not in the load order", x)
            }
            Error::TooManyActivePlugins => write!(f, "Maximum number of active plugins exceeded"),
            Error::InvalidRegex => write!(
                f,
                "Internal error: regex used to parse Morrowind.ini is invalid"
            ),
            Error::DuplicatePlugin => write!(f, "The given plugin list contains duplicates"),
            Error::NonMasterBeforeMaster => write!(
                f,
                "Attempted to load a non-master plugin before a master plugin"
            ),
            Error::GameMasterMustLoadFirst => write!(
                f,
                "The game's implicitly active plugins must load in their harcoded positions"
            ),
            Error::InvalidPlugin(ref x) => write!(f, "The plugin file \"{}\" is invalid", x),
            Error::ImplicitlyActivePlugin(ref x) => write!(
                f,
                "The implicitly active plugin \"{}\" cannot be deactivated",
                x
            ),
            Error::NoLocalAppData => {
                write!(f, "The game's local app data folder could not be detected")
            }
            Error::UnrepresentedHoist(ref plugin, ref master) => write!(
                f,
                "The plugin \"{}\" is a master of \"{}\", which will hoist it",
                plugin, master
            ),
            Error::InstalledPlugin(ref plugin) => write!(
                f,
                "The plugin \"{}\" is installed, so cannot be removed from the load order",
                plugin
            ),
        }
    }
}

impl error::Error for Error {
    fn cause(&self) -> Option<&error::Error> {
        match *self {
            Error::IoError(ref x) => Some(x),
            Error::SystemTimeError(ref x) => Some(x),
            _ => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn game_id_should_map_to_libespm_id_correctly() {
        assert_eq!(EspmId::Morrowind, GameId::Morrowind.to_esplugin_id());
        assert_eq!(EspmId::Oblivion, GameId::Oblivion.to_esplugin_id());
        assert_eq!(EspmId::Skyrim, GameId::Skyrim.to_esplugin_id());
        assert_eq!(EspmId::SkyrimSE, GameId::SkyrimSE.to_esplugin_id());
        assert_eq!(EspmId::SkyrimSE, GameId::SkyrimVR.to_esplugin_id());
        assert_eq!(EspmId::Fallout3, GameId::Fallout3.to_esplugin_id());
        assert_eq!(EspmId::FalloutNV, GameId::FalloutNV.to_esplugin_id());
        assert_eq!(EspmId::Fallout4, GameId::Fallout4.to_esplugin_id());
        assert_eq!(EspmId::Fallout4, GameId::Fallout4VR.to_esplugin_id());
    }

    #[test]
    fn game_id_supports_light_masters_should_be_false_until_fallout_4() {
        assert!(!GameId::Morrowind.supports_light_masters());
        assert!(!GameId::Oblivion.supports_light_masters());
        assert!(!GameId::Skyrim.supports_light_masters());
        assert!(GameId::SkyrimSE.supports_light_masters());
        assert!(GameId::SkyrimVR.supports_light_masters());
        assert!(!GameId::Fallout3.supports_light_masters());
        assert!(!GameId::FalloutNV.supports_light_masters());
        assert!(GameId::Fallout4.supports_light_masters());
        assert!(GameId::Fallout4VR.supports_light_masters());
    }
}
