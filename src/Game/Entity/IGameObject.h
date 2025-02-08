
/**
 * @class IGameObject
 * @brief Interface for a high-level game object abstraction.
 *
 * The IGameObject interface serves as a blueprint for all game objects
 * in the game engine. Game objects encapsulate both state (e.g., position,
 * rotation, velocity) and behavior (e.g., update logic, rendering, submission).
 *
 * Responsibilities:
 * - Manage their position, rotation, velocity, and acceleration.
 * - Implement logic for frame-by-frame updates (e.g., movement, state changes).
 * - Provide functionality for rendering and submitting themselves to external systems.
 *
 * Key Methods:
 * - `Update(float delta)`: Defines per-frame logic such as movement or AI.
 * - `Render() const`: Handles the visual representation of the object.
 * - `Submit() const`: Submits the object to external systems like a render queue
 *   for deferred rendering or physics simulations.
 *
 * Usage:
 * - Derive from this interface to create specific game objects such as players,
 *   enemies, or interactive elements.
 * - Implement the `Update` method to define the object's behavior.
 * - Implement the `Render` and `Submit` methods to handle how the object
 *   integrates into the rendering pipeline or other subsystems.
 */
class IGameObject
{
public:
   IGameObject() {};

   virtual void Update( float delta ) = 0;
   virtual void Render() const        = 0;
   //virtual void Submit() const = 0;

   const glm::vec3& GetPosition() const { return m_position; }
   const glm::vec3& GetRotation() const { return m_rotation; }
   const glm::vec3& GetVelocity() const { return m_velocity; }
   const glm::vec3& GetAcceleration() const { return m_acceleration; }

protected:
   glm::vec3 m_position { 0.0f };
   glm::vec3 m_rotation { 0.0f };
   glm::vec3 m_velocity { 0.0f };
   glm::vec3 m_acceleration { 0.0f };
};
