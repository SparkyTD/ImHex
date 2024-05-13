#pragma once

class TextEditor;

class AutoCompletion {
public:
    void Initialize(TextEditor *editor);
    bool HandleKeyEvent(TextEditor* editor, bool ctrl, bool alt, bool shift);
    void Render(TextEditor* editor, const ImVec2& cursorPosition);

private:
    void DoAutoComplete(TextEditor* editor, const std::string& autoCompleteValue);
    std::string GetAutocompleteBaseWord(TextEditor* editor);
    static bool IsCharacterDelimiter(char c) ;

private:
    bool mIsOpen = false;
    bool mReloadCandidates = false;
    int mSelectionIndex = 0;

    std::vector<std::string> mAllCandidates;
    std::vector<std::string> mCurrentCandidates;
    std::string mLastSearchKey;
};