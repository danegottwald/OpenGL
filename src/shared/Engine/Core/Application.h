#pragma once

// Forward Declarations
namespace Entity
{
class Registry;
}


class Application
{
public:
   static Application& Get() noexcept
   {
      static Application app;
      return app;
   }

   void Run();

private:
   NO_COPY_MOVE( Application )

   Application() noexcept;
   ~Application() noexcept;

   std::unique_ptr< Entity::Registry > m_pRegistry;
};
