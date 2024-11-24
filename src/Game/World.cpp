#include "World.h"

#include "Camera.h"
#include "Renderer/Shader.h"

std::vector<float> vt;
std::vector<unsigned int> it;
std::unique_ptr< Shader > pShader;

World::World()
    : m_VAID( 0 ), m_VBID( 0 ), m_IBID( 0 )
{
    glCreateVertexArrays( 1, &m_VAID );
    glBindVertexArray( m_VAID );

    glCreateBuffers( 1, &m_VBID );
    glBindBuffer( GL_ARRAY_BUFFER, m_VBID );
    pShader = std::make_unique< Shader >( "Basic.glsl" );
}

void World::Setup()
{
    // '1000' is how many Vertex structs to allocate space for
    glBufferData( GL_ARRAY_BUFFER, 100000, nullptr, GL_DYNAMIC_DRAW );

    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), ( const void* )nullptr );

    glCreateBuffers( 1, &m_IBID );
    vt.clear();
    it.clear();
    std::ifstream obj( "./res/models/monkey.obj" );
    std::string data;
    while( std::getline( obj, data ) )
    {
        if( data[ 0 ] == 'v' )
        {
            float x, y, z;
            sscanf( data.c_str(), "v %f %f %f", &x, &y, &z );
            // std::cout << x << " " << y << " " << z << std::endl;
            vt.insert( vt.end(), { x, y, z } );
        }
        else if( data[ 0 ] == 'f' )
        {
            unsigned int x, y, z;
            sscanf( data.c_str(), "f %u %u %u", &x, &y, &z );
            // std::cout << x << " " << y << " " << z << std::endl;
            it.insert( it.end(), { x - 1, y - 1, z - 1 } );
        }
    }
    std::cout << "Vertices: " << vt.size() / 3 << std::endl;
    std::cout << "Indices: " << it.size() / 3 << std::endl;

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBID );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, it.size() * sizeof( unsigned int ), it.data(), GL_STATIC_DRAW );

    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}

void World::Tick( float delta )
{
    // update chunks from vector chunkupdates

    // update voxels then put in vector chunkupdates
}

// Render the world from provided perspective
void World::Render( const Camera& camera )
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glBindBuffer( GL_ARRAY_BUFFER, m_VBID );
    glBufferSubData( GL_ARRAY_BUFFER, 0, vt.size() * sizeof( float ), vt.data() );

    // Bind this shader for OpenGL to use
    pShader->Bind();

    // Set the uniforms in the shader program
    pShader->SetUniform4f( "u_vec4Color", 0.5f, 0.0f, 0.0f, 1.0f );
    pShader->SetUniformMat4f( "u_MVP", camera.GetProjectionView() );

    // m_Texture->Bind(0);
    // m_Shader->SetUniform1i("u_Texture", 0);

    glBindVertexArray( m_VAID );
    glDrawElements( GL_TRIANGLES, it.size(), GL_UNSIGNED_INT, nullptr );

    // maybe use below
    glBindTexture( GL_TEXTURE_2D, 0 );
    glBindVertexArray( 0 );
    glUseProgram( 0 );

    // New stuff below
    // render chunks

    // render all entities
}
