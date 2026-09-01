#include "App.h"

#include <stdexcept>
#include <deque>

#include "imgui-SFML.h"
#include "imgui.h"

App::App() : window_(sf::VideoMode({900, 600}), "RPS - Kamien Papier Nozyce") {
    window_.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window_)) {
        throw std::runtime_error("Nie udalo sie zainicjalizowac ImGui-SFML");
    }

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
            window_.close();
        }
    }
}

void App::update() {
    ImGui::SFML::Update(window_, deltaClock_.restart());
}

void App::draw() {
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("RPS", nullptr, flags);

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
    ImGui::ProgressBar(r.winRateND(), ImVec2(-1, 0), "vs losowy");
    ImGui::ProgressBar(a.winRateND(), ImVec2(-1, 0), "vs AI");

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

    ImGui::End();
}

void App::playRound(Move playerMove) {
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
