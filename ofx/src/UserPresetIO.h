// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <Windows.h>
#include <commdlg.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <optional>
#include "UserPresetConfig.h"

namespace userpreset {

inline std::optional<std::filesystem::path> chooseFile(bool save)
{
    std::array<wchar_t, 32768> filename {};
    OPENFILENAMEW dialog {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFilter = L"OpenEmulsion Presets (*.oepreset)\0*.oepreset\0JSON Files (*.json)\0*.json\0\0";
    dialog.nFilterIndex = 1;
    dialog.lpstrFile = filename.data();
    dialog.nMaxFile = static_cast<DWORD>(filename.size());
    dialog.lpstrDefExt = L"oepreset";
    dialog.lpstrTitle = save ? L"Save OpenEmulsion Preset" : L"Load OpenEmulsion Preset";
    dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_DONTADDTORECENT |
        (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))
        return std::filesystem::path(filename.data());
    if (CommDlgExtendedError() != 0) throw std::runtime_error("Could not open the preset file dialog.");
    return std::nullopt;
}

inline Snapshot loadFile(const std::filesystem::path& path)
{
    std::ifstream file(path,std::ios::binary);
    if (!file) throw std::runtime_error("Could not open the preset file.");
    std::string contents(MaximumBytes + 1,'\0');
    file.read(contents.data(),static_cast<std::streamsize>(contents.size()));
    if (file.bad()) throw std::runtime_error("Could not read the preset file.");
    contents.resize(static_cast<size_t>(file.gcount()));
    return parse(contents);
}

inline void saveFile(const std::filesystem::path& path, const Snapshot& preset)
{
    const auto contents = serialize(preset);
    static std::atomic<unsigned long long> sequence {0};
    std::filesystem::path temporary;
    HANDLE file = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 32; ++attempt) {
        temporary = path;
        temporary += L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." +
            std::to_wstring(GetTickCount64()) + L"." + std::to_wstring(sequence.fetch_add(1));
        file = CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (file != INVALID_HANDLE_VALUE || GetLastError() != ERROR_FILE_EXISTS) break;
    }
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Could not create the preset file in this folder.");
    DWORD written = 0;
    const bool saved = WriteFile(file,contents.data(),static_cast<DWORD>(contents.size()),&written,nullptr) &&
        written == contents.size() && FlushFileBuffers(file);
    const bool closed = CloseHandle(file) != 0;
    // Write beside the target, then replace it only after the complete file is flushed.
    if (!saved || !closed || !MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary.c_str());
        throw std::runtime_error("Could not save the complete preset; the previous file was not replaced.");
    }
}

} // namespace userpreset
