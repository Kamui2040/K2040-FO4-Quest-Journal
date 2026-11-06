// Source-only settings persistence test. Uses a test-only Windows API shim.
#include "JournalSettings.h"
#include <windows.h>  // test-only LocalAppData simulation
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

int main() {
    namespace fs = std::filesystem;
    using namespace K2040::Settings;
    const auto previous = fs::current_path();
    const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto testRoot = fs::temp_directory_path() / ("k2040_journal_preferences_" + suffix);
    fs::create_directories(testRoot);
    fs::current_path(testRoot);

    Load();
    assert(Current().completed == "#74e398");
    assert(Current().theme == "pipboy");
    assert(Current().journalTheme == "pipboy");
    assert(Current().sidebarWidth == 240);
    assert(Current().questListWidth == 360);
    assert(!Current().trackerOverridePhosphor); // safe default for older INIs
    assert(!Current().redirectQuestKey);
    assert(!Change("completed:bogus"));
    assert(!Change("completed:#12345"));
    assert(!Change("sidebarWidth:159"));
    assert(!Change("questListWidth:561"));
    assert(Change("redirectQuestKey:true"));
    assert(Current().redirectQuestKey);
    assert(!Change("redirectQuestKey:yes"));
    assert(Serialize().find("\"redirectQuestKey\":true")!=std::string::npos);
    assert(Change("redirectQuestKey:false"));
    assert(!Current().redirectQuestKey);
    assert(Change("theme:custom"));
    assert(Change("accent:#abcdef"));
    assert(Change("completed:#12a3ef"));
    assert(Change("sidebarWidth:320"));
    assert(Change("questListWidth:470"));
    assert(Change("trackerShowCompleted:true"));
    assert(!Change("trackerOverridePhosphor:1"));
    assert(!Change("trackerOverridePhosphor:TRUE"));
    assert(Change("trackerOverridePhosphor:true"));
    assert(Change("theme:fallout76"));
    assert(Change("journalTheme:fallout76"));
    assert(Change("journalTheme:custom"));
    assert(Change("journalAccent:#aa55cc"));
    assert(Change("journalPanel:#101520"));
    assert(Change("journalText:#cceeff"));
    assert(Change("journalCompleted:#44ffaa"));
    assert(Change("journalOpacity:76"));
    assert(Change("journalFontSize:115"));
    assert(Change("redirectQuestKey:true"));
    assert(!Change("journalTheme:unknown"));
    assert(!Change("journalOpacity:101"));
    assert(!Change("journalAccent:invalid"));
    assert(Save());

    const auto path = testRoot / "Data" / "F4SE" / "Plugins" / "K2040_Quest_Journal.ini";
    assert(fs::is_regular_file(path));
    Load();
    assert(Current().theme == "fallout76");
    assert(Current().accent == "#abcdef");
    assert(Current().completed == "#12a3ef");
    assert(Current().sidebarWidth == 320);
    assert(Current().questListWidth == 470);
    assert(Current().trackerShowCompleted);
    assert(Current().trackerOverridePhosphor);
    assert(Current().journalTheme=="custom");
    assert(Current().theme=="fallout76");
    assert(Current().journalAccent=="#aa55cc");
    assert(Current().journalOpacity==76);
    assert(Current().journalFontSize==115);
    assert(Current().redirectQuestKey);
    const auto payload = Serialize();
    assert(payload.find("\"completed\":\"#12a3ef\"") != std::string::npos);
    assert(payload.find("\"sidebarWidth\":320") != std::string::npos);
    assert(payload.find("\"questListWidth\":470") != std::string::npos);
    assert(payload.find("\"trackerOverridePhosphor\":true") != std::string::npos);
    assert(Change("trackerHotkey:F7"));
    assert(Change("journalHotkey:F10"));
    assert(Change("trackerHotkey:F8"));
    assert(ResetToDefaults());
    assert(Current().completed == "#74e398");
    assert(Current().journalTheme=="pipboy");
    assert(Current().journalOpacity==98 && Current().journalFontSize==100);
    assert(Current().sidebarWidth == 240 && Current().questListWidth == 360);
    assert(Current().journalHotkey == "Shift+G" && Current().trackerHotkey == "G");
    assert(!Current().trackerOverridePhosphor); // reset revokes override
    assert(!Current().redirectQuestKey);
    Load();
    assert(Current().journalHotkey == "Shift+G" && Current().trackerHotkey == "G");
    assert(ChangeAndSave("opacity:81"));
    assert(std::string(LastFailureReason())=="none");
    Load();
    assert(Current().opacity==81);
    // Simulate a non-writable destination as a directory. The transaction must
    // restore the former in-memory preference if the atomic replacement fails.
    fs::remove(path);
    fs::create_directory(path);
    assert(!ChangeAndSave("opacity:67"));
    assert(std::string(LastFailureReason())=="storage");
    assert(Current().opacity==81);
    fs::remove(path);
    assert(Save());  // writes a fresh file after failed replacement

    // Test the real preferred Windows/Proton per-user destination independently
    // of the game's managed Data directory and migrate existing preferences.
    const auto appData=testRoot/"profile"/"AppData"/"Local";
    kTestLocalAppData=appData.wstring();
    Load();
    assert(Current().opacity==81);  // migrated from legacy without editing it
    assert(ChangeAndSave("trackerAnchor:bottom-left"));
    assert(ChangeAndSave("trackerOverridePhosphor:true"));
    assert(ChangeAndSave("theme:pipboy"));
    assert(std::string(LastFailureReason())=="none");
    assert(!ChangeAndSave("theme:invalid"));
    assert(std::string(LastFailureReason())=="invalid");
    const auto userConfig=appData/"K2040"/"FO4-Quest-Journal.ini";
    assert(fs::is_regular_file(userConfig));
    assert(fs::is_regular_file(path));  // legacy file must be preserved
    Load();
    assert(Current().theme=="pipboy");
    assert(Current().trackerAnchor=="bottom-left");
    assert(Current().trackerOverridePhosphor);

    // With an existing user config, later native loads must prefer it even
    // when a different legacy Data INI remains installed.
    assert(ChangeAndSave("trackerWidth:410"));
    Load();
    assert(Current().trackerWidth==410);
    fs::remove(userConfig);
    fs::create_directory(userConfig);
    assert(!ChangeAndSave("trackerOverridePhosphor:false"));
    assert(std::string(LastFailureReason())=="storage");
    assert(Current().trackerOverridePhosphor); // failed writes roll back
    assert(!ChangeAndSave("theme:contrast"));
    assert(std::string(LastFailureReason())=="storage");
    assert(Current().theme=="pipboy");  // failed writes roll back
    fs::remove(userConfig);
    assert(Save());
    kTestLocalAppData.clear();
    fs::current_path(previous);
    fs::remove_all(testRoot);
}
