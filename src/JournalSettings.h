#pragma once
#include <string>
#include <string_view>

namespace K2040::Settings {
    // Global appearance only. Never contains quest IDs or personal save information.
    struct Preferences {
        std::string theme = "pipboy";
        std::string accent = "#66c4ff";
        std::string text = "#f4faff";
        std::string panel = "#0b1218";
        std::string completed = "#74e398";
        // Full-screen journal appearance, separate from miniature tracker.
        std::string journalTheme = "pipboy";
        std::string journalAccent = "#66c4ff";
        std::string journalText = "#f4faff";
        std::string journalPanel = "#0b1218";
        std::string journalCompleted = "#74e398";
        int journalOpacity = 98;
        int journalFontSize = 100;
        int opacity = 94;
        int fontSize = 100;
        std::string density = "comfortable";
        int sidebarWidth = 240;
        int questListWidth = 360;
        std::string trackerAnchor = "top-right";
        int trackerScale = 100;
        int trackerWidth = 340;
        int trackerOpacity = 88;
        bool trackerEnabled = true;
        int trackerMaxQuests = 6;
        bool trackerShowCompleted = false;
        bool trackerAutoPinNew = false;
        bool trackerSyncPipboy = false;
        std::string journalHotkey = "Shift+G";
        std::string trackerHotkey = "G";
    };
    void Load();
    const Preferences& Current();
    // HUD colours are sampled from Fallout's engine and never persisted.
    std::string Serialize(std::string_view hudColor = "#20ff48",
                          std::string_view hudBackgroundColor = "#001407",
                          std::string_view pipboyColor = "");
    bool Change(std::string_view command);
    // Commit validated updates together; restore previous preferences on write failure.
    bool ChangeAndSave(std::string_view command);
    // Fixed non-sensitive status code for UI feedback: invalid or storage.
    const char* LastFailureReason();
    bool Save();
    // Atomic reset of global appearance/controls; never touches per-save quest flags.
    bool ResetToDefaults();
}
