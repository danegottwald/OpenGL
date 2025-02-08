#pragma once

class BaseBlock
{
public:
   explicit BaseBlock( uint16_t id ) :
      m_ID( id )
   {}

   virtual void OnBreak() noexcept = 0;

private:
   uint16_t m_ID;
};


class TestBlock : public BaseBlock
{
public:
   TestBlock() :
      BaseBlock( 0 )
   {}

   void OnBreak() noexcept override {}
};
