#include "TextEditor.h"
#include "AutoCompletion.h"

#include <cstdio>
#include <cmath>
#include <cstring>
#include "imgui.h"

void AutoCompletion::Initialize(TextEditor *editor) {
    mAllCandidates.clear();
    mAllCandidates.emplace_back("#include");
    mAllCandidates.emplace_back("#ifdef");
    mAllCandidates.emplace_back("#ifndef");
    mAllCandidates.emplace_back("#endif");
    mAllCandidates.emplace_back("#define");
    mAllCandidates.emplace_back("#error");
    mAllCandidates.emplace_back("#pragma");

    for(const auto& identifier : editor->GetLanguageDefinition().mIdentifiers) {
        if(identifier.first.length() > 1)
            mAllCandidates.emplace_back(identifier.first);
    }

    for(const auto& keyword : editor->GetLanguageDefinition().mKeywords) {
        if(keyword.length() > 1)
            mAllCandidates.emplace_back(keyword);
    }
}

bool AutoCompletion::HandleKeyEvent(TextEditor *editor, bool ctrl, bool alt, bool shift) {
    // Open popup with Ctrl+Space
    if (!mIsOpen && ctrl && ImGui::IsKeyPressed(ImGuiKey_Space)) {
        mReloadCandidates = true;
        mIsOpen = true;
        return false;
    }

    // In the popup is not open, allow TextEditor to handle the keys
    if (!mIsOpen)
        return true;

    // Close popup if already open
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        mSelectionIndex = 0;
        mIsOpen = false;
        return false;
    }

    // Selection down
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if(mSelectionIndex < mCurrentCandidates.size() - 1)
            mSelectionIndex++;
        else
            mSelectionIndex = 0;
        return false;
    }

    // Selection up
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if(mSelectionIndex > 0)
            mSelectionIndex--;
        else
            mSelectionIndex = mCurrentCandidates.size() - 1;
        return false;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        DoAutoComplete(editor, mCurrentCandidates[mSelectionIndex]);
        mSelectionIndex = 0;
        mIsOpen = false;
        return false;
    }

    return true;
}

void AutoCompletion::Render(TextEditor *editor, const ImVec2 &cursorPosition) {
    if (!mIsOpen)
        return;

    const auto lineOffsetX = editor->TextDistanceToLineStart(editor->mState.mCursorPosition);
    auto lineNo = editor->mState.mCursorPosition.mLine;
    const int visibleLines = 10;
    const float lineHeight = ImGui::GetTextLineHeightWithSpacing();

    ImVec2 lineStartScreenPos = ImVec2(cursorPosition.x, cursorPosition.y + float(lineNo + 1) * editor->mCharAdvance.y);
    ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + editor->mTextStart, lineStartScreenPos.y);

    auto wordUnderCursor = GetAutocompleteBaseWord(editor);

    if (mReloadCandidates || mLastSearchKey != wordUnderCursor) {
        printf("[%s]\n", wordUnderCursor.c_str());
        mSelectionIndex = 0;
        mReloadCandidates = false;
        mCurrentCandidates.clear();
        std::copy_if(mAllCandidates.begin(), mAllCandidates.end(), std::back_inserter(mCurrentCandidates), [&wordUnderCursor](const std::string &s) {
            return s.substr(0, wordUnderCursor.size()) == wordUnderCursor;
        });
        mLastSearchKey = wordUnderCursor;
    }

    ImGui::SetNextWindowPos(ImVec2(textScreenPos.x + lineOffsetX, textScreenPos.y));
    ImGui::SetNextWindowSize(ImVec2(200, lineHeight * std::min(visibleLines, int(mCurrentCandidates.size() + 1))));
    if (ImGui::Begin("asd", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
        if (!mCurrentCandidates.empty()) {
            for (int i = 0; i < mCurrentCandidates.size(); i++) {
                bool isSelected = mSelectionIndex == i;
                if (ImGui::Selectable(mCurrentCandidates[i].c_str(), isSelected)) {
                    DoAutoComplete(editor, mCurrentCandidates[i]);
                    mIsOpen = false;
                }

                if (isSelected) {
                    mSelectionIndex = i;
                    const float scrollY = ImGui::GetScrollY();
                    if (lineHeight * i - scrollY < 0) {
                        ImGui::SetScrollHereY(1.0F / visibleLines);
                    } else if (lineHeight * i - scrollY >= lineHeight * (visibleLines - 1)) {
                        ImGui::SetScrollHereY(1.0F / visibleLines * (visibleLines - 1));
                    }
                }
            }
        } else {
            ImGui::TextDisabled("No results");
        }

        ImGui::End();
    }
}

void AutoCompletion::DoAutoComplete(TextEditor *editor, const std::string &autoCompleteValue) {
    if (!mIsOpen)
        return;

    mSelectionIndex = std::min(mSelectionIndex, int(mCurrentCandidates.size() - 1));

    auto c = editor->GetCursorPosition();

    editor->DeleteRange(TextEditor::Coordinates(c.mLine, c.mColumn - mLastSearchKey.length()), c);
    editor->InsertText(autoCompleteValue);
}

std::string AutoCompletion::GetAutocompleteBaseWord(TextEditor *editor) {
    auto c = editor->GetCursorPosition();
    auto currentLine = editor->GetCurrentLineText();
    auto wordEnd = c.mColumn;
    auto wordStart = wordEnd;
    while (wordStart > 0 && !IsCharacterDelimiter(currentLine[wordStart - 1]))
        wordStart--;

    return currentLine.substr(wordStart, wordEnd);
}

bool AutoCompletion::IsCharacterDelimiter(const char c) {
    return isspace(c) || strchr("()[]{}.,-<>;:=\\/+%^&|'\"", c) != nullptr;
}

