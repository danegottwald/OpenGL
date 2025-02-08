#include "Layer.h"

#include "Window.h"

std::vector< float >        v;
std::vector< unsigned int > i;

Layer::Layer() :
   m_VAID( 0 ),
   m_VBID( 0 ),
   m_IBID( 0 )
{
   glCreateVertexArrays( 1, &m_VAID );
   glBindVertexArray( m_VAID );

   glCreateBuffers( 1, &m_VBID );
   glBindBuffer( GL_ARRAY_BUFFER, m_VBID );

   m_Shader.reset( new Shader() );
   // m_Texture.reset(new Texture("cobble.png"));
}

Layer::~Layer()
{
   glDeleteVertexArrays( 1, &m_VAID );
   glDeleteBuffers( 1, &m_VBID );
   glDeleteBuffers( 1, &m_IBID );
}

void Layer::Enable()
{
   // '1000' is how many Vertex structs to allocate space for
   glBufferData( GL_ARRAY_BUFFER, 100000, nullptr, GL_DYNAMIC_DRAW );

   glEnableVertexAttribArray( 0 );
   glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof( float ), static_cast< const void* >( nullptr ) );

   // glEnableVertexAttribArray(1);
   // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const
   // void*)12);

   // Continue for each attribute in Vertex Struct
   // ...

   // generate index buffer in the format depending on how many triangles are
   // wanted

   // std::array<unsigned int, 36> i = {
   //    // front
   //    0, 1, 2,
   //    2, 3, 0,
   //    // right
   //    1, 5, 6,
   //    6, 2, 1,
   //    // back
   //    7, 6, 5,
   //    5, 4, 7,
   //    // left
   //    4, 0, 3,
   //    3, 7, 4,
   //    // bottom
   //    4, 5, 1,
   //    1, 0, 4,
   //    // top
   //    3, 2, 6,
   //    6, 7, 3
   //};

   glCreateBuffers( 1, &m_IBID );
   LoadModel( "monkey.obj" );

   // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBID);
   // glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size() * sizeof(unsigned int),
   // i.data(), GL_STATIC_DRAW);

   // load textures

   glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );
}

void Layer::Draw( float deltaTime )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glBindBuffer( GL_ARRAY_BUFFER, m_VBID );
   glBufferSubData( GL_ARRAY_BUFFER, 0, v.size() * sizeof( float ), v.data() );

   m_Shader->Bind();
   //m_Shader->SetUniformMat4f( "u_MVP", pc.GetProjectionMatrix() * pc.GetViewMatrix() );

   // m_Texture->Bind(0);
   // m_Shader->SetUniform1i("u_Texture", 0);

   glBindVertexArray( m_VAID );
   glDrawElements( GL_TRIANGLES, i.size(), GL_UNSIGNED_INT, nullptr );

   // maybe use below
   glBindTexture( GL_TEXTURE_2D, 0 );
   glBindVertexArray( 0 );
   glUseProgram( 0 );
}

void Layer::LoadModel( const std::string& filename )
{
   v.clear();
   i.clear();
   std::ifstream obj( filename );
   std::string   data;
   while( std::getline( obj, data ) )
   {
      if( data[ 0 ] == 'v' )
      {
         float x, y, z;
         sscanf( data.c_str(), "v %f %f %f", &x, &y, &z );
         // std::cout << x << " " << y << " " << z << std::endl;
         v.push_back( x );
         v.push_back( y );
         v.push_back( z );
      }
      else if( data[ 0 ] == 'f' )
      {
         unsigned int x, y, z;
         sscanf( data.c_str(), "f %u %u %u", &x, &y, &z );
         x -= 1;
         y -= 1;
         z -= 1;
         // std::cout << x << " " << y << " " << z << std::endl;
         i.push_back( x );
         i.push_back( y );
         i.push_back( z );
      }
   }
   std::cout << "Vertices: " << v.size() / 3 << std::endl;
   std::cout << "Indices: " << i.size() / 3 << std::endl;

   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_IBID );
   glBufferData( GL_ELEMENT_ARRAY_BUFFER, i.size() * sizeof( unsigned int ), i.data(), GL_STATIC_DRAW );
}
