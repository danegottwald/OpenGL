
#pragma once

class GuiOverlay {
private:
    bool m_Active;

public:
    GuiOverlay();
    ~GuiOverlay();
	
    void Attach();
    void Detach();
    void Begin();
    void End();
    
};

