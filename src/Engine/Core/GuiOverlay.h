#pragma once

class GuiOverlay {
private:
    bool m_Active;

public:
    GuiOverlay();
    ~GuiOverlay();

    void Attach();
    void Begin();
    void End();
    void Detach();

};
