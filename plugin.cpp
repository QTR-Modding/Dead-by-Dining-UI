#include "SimpleIni.h"
#include "SkyPrompt/API.hpp"

namespace {
    TESGlobal* hotkey = nullptr;
    std::vector<BGSKeyword*> allowedFood;
    SkyPromptAPI::Prompt poisonPrompt{"$DBDUI_ADD", 0, 0, SkyPromptAPI::PromptType::kHint};
    std::array prompts = {poisonPrompt};
    SkyPromptAPI::ClientID clientID;
    SkyPromptAPI::HandshakeKey handshake_key = 0x44444754; // DDGT
    std::pair<INPUT_DEVICE, uint32_t> key{INPUT_DEVICE::kKeyboard, 0};
    bool bFloatingPrompt = true;

    bool CanBePoisoned(const TESBoundObject* form) { return form->HasKeywordInArray(allowedFood, false); }

    class MyPromptSink : public SkyPromptAPI::PromptSink {
    public:
        [[nodiscard]] std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return prompts; };

        void ProcessEvent(SkyPromptAPI::PromptEvent) const override {
        }
    };

    MyPromptSink g_PromptSink;

    struct mySink : BSTEventSink<CrosshairRefEvent> {
        BSEventNotifyControl ProcessEvent(const CrosshairRefEvent* event,
                                          BSTEventSource<CrosshairRefEvent>*) override {
            static bool bShowing = false;
            if (bShowing) {
                SkyPromptAPI::RemovePrompt(&g_PromptSink, clientID);
                bShowing = false;
            }
            if (const auto ref = event->crosshairRef) {
                if (const auto* base = ref ? ref->GetBaseObject() : nullptr) {
                    if (base->GetFormType() == FormType::AlchemyItem && CanBePoisoned(base)) {
                        if (bFloatingPrompt) {
                            prompts[0].refid = ref->GetFormID();
                        }
                        key.second = static_cast<uint32_t>(hotkey->value);
                        prompts[0].button_key = std::span{&key, 1};
                        if (!SkyPromptAPI::SendPrompt(&g_PromptSink, clientID)) {
                            //
                        }
                        bShowing = true;
                    }
                }
            }

            return BSEventNotifyControl::kContinue;
        }
    };

    void setup() {
        hotkey = TESForm::LookupByEditorID<TESGlobal>("DBD_Hotkey");
        if (hotkey) {
            static mySink g_EventSink;
            GetCrosshairRefEventSource()->AddEventSink(&g_EventSink);
            allowedFood.reserve(1);
            allowedFood.push_back(TESForm::LookupByEditorID<BGSKeyword>("DBD_Drink"));
            clientID = SkyPromptAPI::RequestClientID();
            if (!SkyPromptAPI::RequestHandshake(clientID, handshake_key, handshake_key)) {
                // Handshake request failed
            }

            CSimpleIniA ini;
            const std::string filePath = "Data/SKSE/Plugins/DeadByDiningUI.ini";
            if (ini.LoadFile(filePath.c_str()) == SI_OK) {
                bFloatingPrompt = ini.GetBoolValue("Main", "bFloatingPrompt", true);
            }
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    Init(skse);

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message) {
        if (message->type == MessagingInterface::kDataLoaded) {
            setup();
        }
    });

    return true;
}