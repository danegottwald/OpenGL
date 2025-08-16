
#include "Registry.h"

namespace Entity
{

EntityHandle::~EntityHandle()
{
   if( m_pRegistry )
      m_pRegistry->Destroy( m_entity );
}

} // namespace Entity
