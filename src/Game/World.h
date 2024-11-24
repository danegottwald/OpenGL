#pragma once

using GLuint = unsigned int;

// Forward Declarations
class Camera;

class World
{
public:
    World();

    void Setup();
    void Tick( float delta );
    void Render( const Camera& camera );

private:
    // renderer stuff
    GLuint m_VAID, m_VBID, m_IBID;

    // All relevant world data
    //      Generate terrain on construction?
    // Maybe list of all entities? then Tick them in Tick()?
    // std::vector< Entity > m_EntityList;
};
