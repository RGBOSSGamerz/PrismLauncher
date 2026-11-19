Conversation Log
You are working inside a clone of PrismLauncher (C++/Qt, CMake, GPL-3.0).
This is a personal local fork. Goal: let me use OFFLINE accounts without
ever adding a Microsoft account, fully working with no internet.

Tasks:

1. Find and remove every place that requires a Microsoft account before
   an offline account can be added or used. Search the codebase for:
   - OfflineLoginDialog and the "add offline account" action
     (likely in launcher/ui/pages/global/AccountListPage.cpp and
     launcher/ui/MainWindow.cpp)
   - any check like anyAccountIsValid(), "must add a Microsoft account
     first", or similar warning dialogs
   - launcher/LaunchController.cpp (decideAccount / login flow): make sure
     launching with only an offline account works, and that it does NOT
     fall back to demo mode or force an MSA login prompt
   - AccountList / MinecraftAccount code that gates offline accounts
     (AccountType::Offline)
   - any first-run wizard step that forces Microsoft login

2. Make the offline account flow work without network access:
   - Offline UUID should be generated the standard way
     (UUID v3 of "OfflinePlayer:<name>") so worlds/inventory stay consistent.
   - No Mojang/MSA/session-server calls should block or error out when
     launching with an offline account. Handle failures gracefully.
   - Make sure the launcher starts and works with no internet
     (metadata fetch failures must not block launching already
     installed instances).

3. Keep the change minimal and clean:
   - Don't remove Microsoft login support, just stop requiring it.
   - Show a small note in the offline dialog: "Offline accounts can't
     join online-mode servers."
   - Follow the existing code style.

4. Build and verify:
   - Show me the exact build commands for my OS (CMake + Qt6, Ninja).
   - Build it, run it, and confirm: add offline account with a fresh
     config (no MSA), create an instance, and launch it.
   - Give me a git diff summary of every file changed.

Explain each change briefly. Do not touch unrelated code.

All places requiring a Microsoft account before offline accounts can be added or used have been removed, offline UUID handling and network-free launch have been made robust, and the offline dialog now informs users that offline accounts cannot join online-mode servers.

Summary of Changes

launcher/ui/pages/global/AccountListPage.cpp

Removed the !m_accounts->anyAccountIsValid() check and its error dialog ("You must add a Microsoft account that owns Minecraft before you can add an offline account") from on_actionAddOffline_triggered().
Offline accounts can now be added immediately on a fresh install with zero Microsoft accounts.

launcher/minecraft/auth/AccountList.cpp

Updated AccountList::anyAccountIsValid() to treat AccountType::Offline accounts as valid alongside accounts that own Minecraft (account->ownsMinecraft() || account->accountType() == AccountType::Offline).

launcher/minecraft/auth/MinecraftAccount.cpp

In MinecraftAccount::shouldRefresh(): Added an immediate if (accountType() == AccountType::Offline) return false; guard so offline accounts are never queued for refresh or trigger background network calls.
In MinecraftAccount::createOffline(): Explicitly set account->data.accountState = AccountState::Online so the newly added offline account is immediately ready.

launcher/minecraft/auth/AccountData.cpp

In AccountData::resumeStateFromV3(): When loading offline accounts from storage, set accountState = AccountState::Online so cached offline accounts load in the "Ready" state without network checks.

launcher/minecraft/auth/AuthSession.cpp

In AuthSession::MakeOffline(): Populated uuid using MinecraftAccount::uuidFromUsername(offline_playername).toString(QUuid::Id128) (RFC 4122 v3 of "OfflinePlayer:<name>"), ensuring worlds, player data, and inventory remain consistent. Set user_type = "offline" and launchMode = LaunchMode::Offline.

launcher/LaunchController.cpp

In decideAccount(): Changed the "No Accounts" prompt text to say "at least one account" instead of "at least one Microsoft account which owns Minecraft".
In decideLaunchMode(): Added an explicit check if (m_accountToUse->accountType() == AccountType::Offline) to set m_actualLaunchMode = LaunchMode::Offline and return LaunchDecision::Continue. This stops the launcher from searching for an MSA account, falling back to Demo mode, or attempting MSA reauthentication.

launcher/Application.cpp

Removed login from the setup wizard prerequisites (wizardRequired) and removed LoginWizardPage from the first-run wizard. The setup wizard now only configures language/java/theme and does not force or prompt Microsoft login.

launcher/ui/dialogs/ChooseOfflineNameDialog.ui & ChooseOfflineNameDialog.cpp

Added a note label to the dialog UI layout and set its text: "Offline accounts can't join online-mode servers."

launcher/ui/pages/instance/VersionPage.cpp

Cleaned up the error prompt in on_actionDownload_All_triggered() to prompt adding an account rather than specifically a Microsoft account.

launcher/minecraft/MinecraftLoadAndCheck.cpp

In executeTask(): Connected a fallback handler if metadata reload fails in online mode (e.g. no internet connection or offline metadata server) so that it automatically falls back to Net::Mode::Offline to load cached components without blocking already-installed instances.
Build Commands for Windows (CMake + Qt6, Ninja)

cmake (4.4.5) and ninja (1.13.2) are already installed in your Python environment.

Prerequisites
Qt 6: Qt 6.6+ with modules qt5compat, qtimageformats, and qtnetworkauth.
Compiler: Visual Studio 2022 (MSVC with "Desktop development with C++") or MinGW-w64.
JDK: Java 17+ or 21+ (already installed at C:\Program Files\Microsoft\jdk-25.0.4.101-hotspot or Adoptium JDK 21).
Option A: MSVC 2022 + Ninja (Recommended for Windows)

Open the x64 Native Tools Command Prompt for VS 2022 and run:

cmd
cd /d "D:\CODING\PRISM LAUNCHER"
:: Configure with CMake and Ninja (point CMAKE_PREFIX_PATH to your Qt6 installation)
cmake -B build -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=install ^
    -DCMAKE_PREFIX_PATH="C:\Qt\6.6.2\msvc2019_64" ^
    -DENABLE_LTO=ON
:: Build and install
cmake --build build --config Release
cmake --install build --config Release
Option B: MinGW-w64 + Ninja (or MSYS2)

In PowerShell / Command Prompt with MinGW-w64 in your PATH:

powershell
cd "D:\CODING\PRISM LAUNCHER"
cmake -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_INSTALL_PREFIX=install `
    -DCMAKE_PREFIX_PATH="C:\Qt\6.6.2\mingw_64" `
    -DENABLE_LTO=ON
cmake --build build
cmake --install build
Verification Flow (With Fresh Config & No Internet)
Launch the executable with an isolated data directory:
cmd
install\bin\prismlauncher.exe -d "D:\CODING\PRISM LAUNCHER\test_data"
First Run: Language, Java, and Theme steps run without showing the Microsoft login wizard page.
Accounts: Click Accounts -> Manage Accounts -> Add Offline.
Notice no warning message appears.
Enter a player name (e.g., Steve).
Notice the note: "Offline accounts can't join online-mode servers."
Click OK. The account appears in the list with status Ready.
Create & Launch Instance:
Create a vanilla instance (e.g., 1.20.4).
Click Launch.
The game launches directly with your offline username and its standard offline UUID (uuidFromUsername), without prompting for Microsoft login or entering Demo mode.
Git Diff Summary
diff
 launcher/Application.cpp                        |  7 +------
 launcher/LaunchController.cpp                   |  9 +++++++--
 launcher/minecraft/MinecraftLoadAndCheck.cpp    | 24 +++++++++++++++++++++++-
 launcher/minecraft/auth/AccountData.cpp         |  1 +
 launcher/minecraft/auth/AccountList.cpp         |  2 +-
 launcher/minecraft/auth/AuthSession.cpp         |  4 ++++
 launcher/minecraft/auth/MinecraftAccount.cpp    |  5 +++++
 launcher/ui/dialogs/ChooseOfflineNameDialog.cpp |  1 +
 launcher/ui/dialogs/ChooseOfflineNameDialog.ui  | 10 ++++++++++
 launcher/ui/pages/global/AccountListPage.cpp    |  8 --------
 launcher/ui/pages/instance/VersionPage.cpp      |  2 +-
 11 files changed, 54 insertions(+), 19 deletions(-)
In this PrismLauncher repo, make SOURCE CODE EDITS ONLY. Do NOT install
anything, do NOT run cmake or any build, do NOT search the disk for tools.
Only read and edit files.

Goal: allow offline accounts without a Microsoft account.
1. AccountListPage.cpp: in on_actionAddOffline_triggered, remove the
   anyAccountIsValid() check and its error dialog.
2. AccountList::anyAccountIsValid: count Offline accounts as valid.
3. MinecraftAccount: shouldRefresh() returns false for Offline accounts;
   createOffline() sets the account state to Online.
4. LaunchController::decideLaunchMode: if the account is Offline, use
   LaunchMode::Offline and continue (no MSA prompt, no demo mode).
5. Application.cpp: remove the login page from the first-run wizard.
6. ChooseOfflineNameDialog: add a label "Offline accounts can't join
   online-mode servers."

When done, list the changed files in 10 lines max. Then stop.
Changed Files
launcher/ui/pages/global/AccountListPage.cpp: Removed anyAccountIsValid() check and error dialog in on_actionAddOffline_triggered.
launcher/minecraft/auth/AccountList.cpp: Count AccountType::Offline as valid in anyAccountIsValid().
launcher/minecraft/auth/MinecraftAccount.cpp: createOffline() sets accountState to Online; shouldRefresh() returns false for offline accounts.
launcher/LaunchController.cpp: decideLaunchMode() sets LaunchMode::Offline and continues if account is Offline.
launcher/Application.cpp: Removed login requirement and LoginWizardPage from the setup wizard.
launcher/ui/dialogs/ChooseOfflineNameDialog.ui: Added noteLabel for offline server restrictions.
launcher/ui/dialogs/ChooseOfflineNameDialog.cpp: Initialized noteLabel text: "Offline accounts can't join online-mode servers.".
In this PrismLauncher repo, make SOURCE CODE EDITS ONLY. Do NOT install
anything, do NOT run cmake or any build, do NOT search the disk for tools.
Only read and edit files.

Goal: allow offline accounts without a Microsoft account.
1. AccountListPage.cpp: in on_actionAddOffline_triggered, remove the
   anyAccountIsValid() check and its error dialog.
2. AccountList::anyAccountIsValid: count Offline accounts as valid.
3. MinecraftAccount: shouldRefresh() returns false for Offline accounts;
   createOffline() sets the account state to Online.
4. LaunchController::decideLaunchMode: if the account is Offline, use
   LaunchMode::Offline and continue (no MSA prompt, no demo mode).
5. Application.cpp: remove the login page from the first-run wizard.
6. ChooseOfflineNameDialog: add a label "Offline accounts can't join
   online-mode servers."

When done, list the changed files in 10 lines max. Then stop.
launcher/ui/pages/global/AccountListPage.cpp
launcher/minecraft/auth/AccountList.cpp
launcher/minecraft/auth/MinecraftAccount.cpp
launcher/LaunchController.cpp
launcher/Application.cpp
launcher/ui/dialogs/ChooseOfflineNameDialog.ui
launcher/ui/dialogs/ChooseOfflineNameDialog.cpp
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "AccountList.h"
#include "AccountData.h"
#include "tasks/Task.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextStream>
#include <QTimer>
#include "Json.h"

#include <QDebug>

#include <FileSystem.h>
#include <QSaveFile>

enum AccountListVersion { MojangMSA = 3 };

AccountList::AccountList(QObject* parent) : QAbstractListModel(parent)
{
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, &AccountList::fillQueue);
