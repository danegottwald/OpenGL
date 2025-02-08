#include "World.h"

#include "Camera.h"
#include "Renderer/Shader.h"

std::vector< float >        vertexData;
std::vector< unsigned int > indexData;
std::unique_ptr< Shader >   pShader;

World::World() :
   m_VAID( 0 ),
   m_VBID( 0 ),
   m_IBID( 0 )
{
   pShader = std::make_unique< Shader >();
}

// clang-format off
   //size_t vertexDataLength = 6; // (3 position, 3 normal)
   //vertexData = {
   //    // Positions          // Normals
   //    // Front face
   //    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v1
   //     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v2
   //     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v3
   //    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v4
   //
   //    // Back face
   //    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v5
   //    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v6
   //     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v7
   //     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v8
   //
   //    // Left face
   //    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  // v9
   //    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  // v10
   //    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  // v11
   //    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  // v12
   //
   //    // Right face
   //     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  // v13
   //     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  // v14
   //     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  // v15
   //     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  // v16
   //
   //    // Top face
   //    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  // v17
   //    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  // v18
   //     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  // v19
   //     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  // v20
   //
   //    // Bottom face
   //    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  // v21
   //     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  // v22
   //     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  // v23
   //    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f   // v24
   //};
   //
   //indexData = { // Front face
   //              0, 1, 2, 2, 3, 0,
   //              // Back face
   //              4, 5, 6, 6, 7, 4,
   //              // Left face
   //              8, 9, 10, 10, 11, 8,
   //              // Right face
   //              12, 13, 14, 14, 15, 12,
   //              // Top face
   //              16, 17, 18, 18, 19, 16,
   //              // Bottom face
   //              20, 21, 22, 22, 23, 20
   //};
// clang-format on

//struct Vertex
//{
//   glm::vec3 position; // Position of the vertex
//   glm::vec3 normal;   // Normal for the vertex
//};


void World::Setup()
{
   vertexData.clear();
   indexData.clear();

   const float cubeSize = 1.0f; // Each cube is 1x1x1 in size

   // Loop through the 6x6x6 chunk grid
   for( int x = 0; x < 6; ++x ) // length
   {
      for( int y = 0; y < 1; ++y ) // height
      {
         for( int z = 0; z < 6; ++z ) // width
         {
            glm::vec3 offset( x * cubeSize, y * cubeSize, z * cubeSize ); // Corrected offset

            for( int face = 0; face < 6; ++face )
            {
               glm::vec3 normal;
               glm::vec3 vertices[ 4 ];

               switch( face )
               {
                  case 0: // Front face
                     normal        = glm::vec3( 0.0f, 0.0f, 1.0f );
                     vertices[ 0 ] = glm::vec3( -0.5f, -0.5f, 0.5f );
                     vertices[ 1 ] = glm::vec3( 0.5f, -0.5f, 0.5f );
                     vertices[ 2 ] = glm::vec3( 0.5f, 0.5f, 0.5f );
                     vertices[ 3 ] = glm::vec3( -0.5f, 0.5f, 0.5f );
                     break;

                  case 1: // Back face
                     normal        = glm::vec3( 0.0f, 0.0f, -1.0f );
                     vertices[ 0 ] = glm::vec3( -0.5f, -0.5f, -0.5f );
                     vertices[ 1 ] = glm::vec3( -0.5f, 0.5f, -0.5f );
                     vertices[ 2 ] = glm::vec3( 0.5f, 0.5f, -0.5f );
                     vertices[ 3 ] = glm::vec3( 0.5f, -0.5f, -0.5f );
                     break;

                  case 2: // Left face
                     normal        = glm::vec3( -1.0f, 0.0f, 0.0f );
                     vertices[ 0 ] = glm::vec3( -0.5f, -0.5f, -0.5f );
                     vertices[ 1 ] = glm::vec3( -0.5f, -0.5f, 0.5f );
                     vertices[ 2 ] = glm::vec3( -0.5f, 0.5f, 0.5f );
                     vertices[ 3 ] = glm::vec3( -0.5f, 0.5f, -0.5f );
                     break;

                  case 3: // Right face
                     normal        = glm::vec3( 1.0f, 0.0f, 0.0f );
                     vertices[ 0 ] = glm::vec3( 0.5f, -0.5f, -0.5f );
                     vertices[ 1 ] = glm::vec3( 0.5f, 0.5f, -0.5f );
                     vertices[ 2 ] = glm::vec3( 0.5f, 0.5f, 0.5f );
                     vertices[ 3 ] = glm::vec3( 0.5f, -0.5f, 0.5f );
                     break;

                  case 4: // Top face
                     normal        = glm::vec3( 0.0f, 1.0f, 0.0f );
                     vertices[ 0 ] = glm::vec3( -0.5f, 0.5f, -0.5f );
                     vertices[ 1 ] = glm::vec3( -0.5f, 0.5f, 0.5f );
                     vertices[ 2 ] = glm::vec3( 0.5f, 0.5f, 0.5f );
                     vertices[ 3 ] = glm::vec3( 0.5f, 0.5f, -0.5f );
                     break;

                  case 5: // Bottom face
                     normal        = glm::vec3( 0.0f, -1.0f, 0.0f );
                     vertices[ 0 ] = glm::vec3( -0.5f, -0.5f, -0.5f );
                     vertices[ 1 ] = glm::vec3( 0.5f, -0.5f, -0.5f );
                     vertices[ 2 ] = glm::vec3( 0.5f, -0.5f, 0.5f );
                     vertices[ 3 ] = glm::vec3( -0.5f, -0.5f, 0.5f );
                     break;
               }

               // Add each face's vertices to the vertex data (with offset applied)
               for( int i = 0; i < 4; ++i )
               {
                  vertexData.push_back( vertices[ i ].x + offset.x );
                  vertexData.push_back( vertices[ i ].y + offset.y );
                  vertexData.push_back( vertices[ i ].z + offset.z );
                  vertexData.push_back( normal.x );
                  vertexData.push_back( normal.y );
                  vertexData.push_back( normal.z );
               }

               // Add the indices (using a counter for the current vertex base)
               unsigned int baseIndex = vertexData.size() / 6 - 4; // Adjusted index calculation
               indexData.push_back( baseIndex + 0 );
               indexData.push_back( baseIndex + 1 );
               indexData.push_back( baseIndex + 2 );
               indexData.push_back( baseIndex + 2 );
               indexData.push_back( baseIndex + 3 );
               indexData.push_back( baseIndex + 0 );
            }
         }
      }
   }

   std::cout << "Vertex Data Size: " << vertexData.size() << std::endl;
   std::cout << "Index Data Size: " << indexData.size() << std::endl;

   // Create and bind Vertex Array Object (VAO)
   glCreateVertexArrays( 1, &m_VAID );
   glBindVertexArray( m_VAID );

   // Create and bind Vertex Buffer Object (VBO)
   glCreateBuffers( 1, &m_VBID );
   glBindBuffer( GL_ARRAY_BUFFER, m_VBID );
   glBufferData( GL_ARRAY_BUFFER, vertexData.size() * sizeof( float ), vertexData.data(), GL_STATIC_DRAW );

   // Create and bind Index Buffer Object (IBO)
   glGenBuffers( 1, &m_IBID );
   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBID );
   glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexData.size() * sizeof( unsigned int ), indexData.data(), GL_STATIC_DRAW );

   // Define Vertex Attributes (Position)
   glEnableVertexAttribArray( 0 );
   glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), ( const void* )0 );

   // Define Vertex Attributes (Normals)
   glEnableVertexAttribArray( 1 );
   glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), ( const void* )( 3 * sizeof( float ) ) );
}

glm::vec3 lightPos;
void      World::Tick( float delta )
{
   // Update time by the delta, loop back every 5 seconds
   static float time = 0.0f; // Accumulates time passed
   time += delta;
   if( time > 5.0f )
      time -= 5.0f; // Reset time after 5 seconds for continuous orbit

   // Angle based on the elapsed time and duration of the orbit (5 seconds)
   // Set the light position to orbit around (0, 0, 0) in the X-Z plane
   float angle = glm::two_pi< float >() * ( time / 5.0f ); // 360 degrees in radians (two_pi)
   lightPos    = glm::vec3( glm::cos( angle ) * 5.0f, 2.0f, glm::sin( angle ) * 5.0f );
}

// Render the world from provided perspective
void World::Render( const Camera& camera )
{
   // 1. Clear the color and depth buffers
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // 2. Update vertex buffer data (if changed)
   //glBindBuffer( GL_ARRAY_BUFFER, m_VBID ); // Bind the Vertex Buffer Object (VBO)
   //glBufferSubData( GL_ARRAY_BUFFER, 0, vt.size() * sizeof( float ), vt.data() ); // Update vertex data if changed

   // 3. Bind shader and set uniform values
   pShader->Bind();                                            // Bind the shader program
   pShader->SetUniform( "u_MVP", camera.GetProjectionView() ); // Set Model-View-Projection matrix
   pShader->SetUniform( "u_viewPos", camera.GetPosition() );   // Set camera position
   pShader->SetUniform( "u_color", 0.5f, 0.0f, 0.0f );         // Set color (example: red)
   pShader->SetUniform( "u_lightPos", lightPos );              // Set light position

   // 4. Optional: Bind texture (if any)
   // m_Texture->Bind(0); // Bind texture (if using one)
   // pShader->SetUniform1i("u_Texture", 0); // Set texture unit in shader

   // 5. Bind Vertex Array Object (VAO) and draw elements
   glBindVertexArray( m_VAID );                                                // Bind the Vertex Array Object (VAO)
   glDrawElements( GL_TRIANGLES, indexData.size(), GL_UNSIGNED_INT, nullptr ); // Draw the elements using indices from IBO

   // 6. Unbind VAO and shader program (clean up)
   glBindVertexArray( 0 ); // Unbind VAO
   pShader->Unbind();      // Unbind shader program

   // 7. Optional: Unbind texture (if any)
   // glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture (if using one)

   // 8. Render additional objects (optional, e.g., chunks or entities)
   // renderChunks(); // Example: render chunks
   // renderEntities(); // Example: render entities
}
