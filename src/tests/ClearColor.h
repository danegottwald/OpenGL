#pragma once

#include "Test.h"

namespace TestSpace
{

class ClearColor : public Test
{
private:
   float m_ClearColor[ 4 ];

public:
   ClearColor();
   ~ClearColor();

   void OnUpdate( float deltaTime ) override;
   void OnRender() override;
   void OnImGuiRender() override;
};

}
