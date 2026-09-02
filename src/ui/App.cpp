#include "App.h"

#include <stdexcept>
#include <deque>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cctype>

#include "imgui-SFML.h"
#include "imgui.h"

using namespace std;

std::string sanitizeUsername(const std::string& raw) {
    std::string result;
    for (char c : raw) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            result += c;
        }
    }
    return result;
}

App::App() : window_(sf::VideoMode({900, 600}), "RPS - Kamien Papier Nozyce") {
    window_.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window_)) {
        throw std::runtime_error("Nie udalo sie zainicjalizowac ImGui-SFML");
    }

    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = ImVec4(0.1f, 0.5f, 0.6f, 1.0f);
}

App::~App() {
    ImGui::SFML::Shutdown();
}

void App::run() {
    while (window_.isOpen()) {
        processEvents();
        update();

        window_.clear(sf::Color(30, 30, 30));
        draw();
        ImGui::SFML::Render(window_);
        window_.display();
    }
}

void App::processEvents() {
    while (const auto event = window_.pollEvent()) {
        ImGui::SFML::ProcessEvent(window_, *event);
        if (event->is<sf::Event::Closed>()) {
            if(!wasSaved) {
                pendingUser_ = currentUser_;
                confirmClosePending_ = true;
            } else {
                window_.close();
            }
        }
    }
}

void App::update() {
    ImGui::SFML::Update(window_, deltaClock_.restart());
}

vector<string> App::listUsers() {
    filesystem::create_directories("users");

    vector<string> users;
    for (const auto& entry : filesystem::directory_iterator("users")) {
        if (entry.path().extension() == ".txt") {
            users.push_back(entry.path().stem().string());
        }
    }

    sort(users.begin(), users.end());
    // for (const auto& user : users) {
    //     cout << "Znaleziono uzytkownika: " << user << endl;
    // }
    return users;
}

void App::resetSession() {
    stats_ = Stats();
    lastRandomMove_.reset();
    lastAIMove_.reset();
    roundNumber_ = 0;
    gamesHistory.clear();
}


void App::switchUser(const std::string& username) {
    currentUser_ = username;
    neuralOpponent_.loadFromFile("users/" + username + ".txt");
    resetSession();
    wasSaved = true;
}

void App::requestSwitchUser(const std::string& username) {
    if(!wasSaved) {
        pendingUser_ = username;
        confirmSwitchPending_ = true;
    } else {
        switchUser(username);
    }
}

void App::draw() {
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("RPS", nullptr, flags);

    // save warning popups

    if(confirmSwitchPending_) {
        ImGui::OpenPopup("Nie zapisano sieci");
        confirmSwitchPending_ = false;
    }

    if(ImGui::BeginPopupModal("Nie zapisano sieci", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Nie zapisano sieci dla uzytkownika \"%s\". Czy chcesz zapisac?", currentUser_.c_str());
        ImGui::Separator();

        if (ImGui::Button("Tak, zapisz")) {
            neuralOpponent_.saveToFile("users/" + currentUser_ + ".txt");
            switchUser(pendingUser_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Nie, nie zapisuj")) {
            switchUser(pendingUser_);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Anuluj")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (confirmClosePending_) {
        ImGui::OpenPopup("Zamknac bez zapisu?");
        confirmClosePending_ = false;
    }

    if (ImGui::BeginPopupModal("Zamknac bez zapisu?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Nie zapisano sieci dla uzytkownika \"%s\". Co zrobic?", currentUser_.c_str());
        ImGui::Separator();

        if (ImGui::Button("Zapisz i zamknij")) {
            neuralOpponent_.saveToFile("users/" + currentUser_ + ".txt");
            window_.close();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Zamknij bez zapisu")) {
            window_.close();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Anuluj")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


    // Uzytkownicy menu

    ImGui::Text("Uzytkownik:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::BeginCombo("##User", currentUser_.c_str())){
        for (const std::string& user : listUsers()) {
            bool selected = user == currentUser_;
            if (ImGui::Selectable(user.c_str(), selected)) {
                requestSwitchUser(user);
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if(currentUser_ != "default") {
        if (ImGui::Button("Usun uzytkownika")) {
            ImGui::OpenPopup("Usun uzytkownika?");
        }
    }
    
    if (ImGui::BeginPopupModal("Usun uzytkownika?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Czy na pewno usunac uzytkownika \"%s\"?", currentUser_.c_str());
        ImGui::Separator();

        if (ImGui::Button("Tak, usun")) {
            std::filesystem::remove("users/" + currentUser_ + ".txt");
            switchUser("default");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Anuluj")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Nowy uzytkownik")) {
        showNewUserInput = 1;
        newUserInput[0] = '\0';
    }

    ImGui::SameLine();
    if (showNewUserInput) {
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputText("##Nazwa", newUserInput, sizeof(newUserInput));
        ImGui::SameLine();
        if (ImGui::Button("Utworz") || ImGui::IsKeyPressed(ImGuiKey_Enter, 0)) {
            std::string name = sanitizeUsername(newUserInput);
            if (!name.empty()) {
                requestSwitchUser(name);
                showNewUserInput = 0;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Anuluj") || ImGui::IsKeyPressed(ImGuiKey_Escape, 0)) {
            showNewUserInput = 0;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Gorny rzad: dwa panele obok siebie (losowy przeciwnik / AI).
    const float panelWidth = ImGui::GetContentRegionAvail().x / 2.f - 8.f;

    ImGui::BeginChild("RandomPanel", ImVec2(panelWidth, 160), true);
    ImGui::Text("Przeciwnik losowy");
    ImGui::Separator();
    ImGui::Text("Ostatni ruch: %s", lastRandomMove_ ? moveName(*lastRandomMove_) : "-");
    const Record& r = stats_.vsRandom();
    ImGui::Text("Wygrane: %d  Przegrane: %d  Remisy: %d", r.wins, r.losses, r.draws);
    ImGui::Text("Win rate: %.1f%%", r.winRate() * 100.f);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("AIPanel", ImVec2(panelWidth, 160), true);
    ImGui::Text("AI (uczy sie Ciebie)");
    ImGui::Separator();
    ImGui::Text("Ostatni ruch: %s", lastAIMove_ ? moveName(*lastAIMove_) : "-");
    const Record& a = stats_.vsAI();
    ImGui::Text("Wygrane: %d  Przegrane: %d  Remisy: %d", a.wins, a.losses, a.draws);
    ImGui::Text("Win rate: %.1f%%", a.winRate() * 100.f);
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Porownanie.
    ImGui::Text("Runda: %d", roundNumber_);
    ImGui::ProgressBar(r.winRateND(), ImVec2(-1, 0), ("vs losowy (" + std::to_string(r.winRateND() * 100.f).substr(0, 4) + "%)").c_str());
    ImGui::ProgressBar(a.winRateND(), ImVec2(-1, 0), ("vs AI (" + std::to_string(a.winRateND() * 100.f).substr(0, 4) + "%)").c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Dol: przyciski gracza.
    ImGui::Text("Twoj ruch:");
    const float buttonWidth = ImGui::GetContentRegionAvail().x / 3.f - 8.f;
    if (ImGui::Button("Kamien [1]", ImVec2(buttonWidth, 60)) || ImGui::IsKeyPressed(ImGuiKey_1, 0) || ImGui::IsKeyPressed(ImGuiKey_Keypad1, 0)) playRound(Move::Rock);
    ImGui::SameLine();
    if (ImGui::Button("Papier [2]", ImVec2(buttonWidth, 60)) || ImGui::IsKeyPressed(ImGuiKey_2, 0) || ImGui::IsKeyPressed(ImGuiKey_Keypad2, 0)) playRound(Move::Paper);
    ImGui::SameLine();
    if (ImGui::Button("Nozyce [3]", ImVec2(buttonWidth, 60)) || ImGui::IsKeyPressed(ImGuiKey_3, 0) || ImGui::IsKeyPressed(ImGuiKey_Keypad3, 0)) playRound(Move::Scissors);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float currentWidth = ImGui::GetContentRegionAvail().x - 8.f;
    ImGui::BeginChild("HistoryPanel", ImVec2(currentWidth, 160), true);
    ImGui::Text("Historia gier: ");
    ImGui::Separator();

    ImGui::BeginTable("HistoryTable", 3);
    ImGui::TableSetupColumn("Twoj Ruch");
    ImGui::TableSetupColumn("Ruch losowy");
    ImGui::TableSetupColumn("Ruch AI");
    ImGui::TableHeadersRow();
    for(const GameEvent &ev : gamesHistory){
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("%s", moveName(ev.playersMove));
        ImGui::TableNextColumn(); ImGui::TextColored(ev.vsRandom == Outcome::Win ? ImVec4(0,1,0,1) : (ev.vsRandom == Outcome::Loss ? ImVec4(1,0,0,1) : ImVec4(1,1,1,1)), "%s (%s)", moveName(ev.randomMove), outcomeName(ev.vsRandom));
        ImGui::TableNextColumn(); ImGui::TextColored(ev.vsAI == Outcome::Win ? ImVec4(0,1,0,1) : (ev.vsAI == Outcome::Loss ? ImVec4(1,0,0,1) : ImVec4(1,1,1,1)), "%s (%s)", moveName(ev.aiMove), outcomeName(ev.vsAI));
    }
    ImGui::EndTable();

    ImGui::EndChild();

    ImGui::Spacing();
    if(ImGui::Button("Zapisz siec", ImVec2(100, 30)) || (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S, 0))) {
        neuralOpponent_.saveToFile("users/" + currentUser_ + ".txt");
        wasSaved = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Resetuj statystyki", ImVec2(160, 30))) {
        resetSession();
    }


    ImGui::End();
}

void App::playRound(Move playerMove) {
    wasSaved = false;

    const Move randomMove = randomOpponent_.pick();
    const Move aiMove = neuralOpponent_.predict();
    neuralOpponent_.learn(aiMove, playerMove);
    neuralOpponent_.update(playerMove, aiMove);

    const Outcome vsRandom = resolve(playerMove, randomMove);
    const Outcome vsAI = resolve(playerMove, aiMove);

    stats_.record(vsRandom, vsAI);

    lastRandomMove_ = randomMove;
    lastAIMove_ = aiMove;

    gamesHistory.push_front({playerMove, randomMove, aiMove, vsRandom, vsAI});
    if(gamesHistory.size() > 100) gamesHistory.pop_back();
    roundNumber_++;
}
