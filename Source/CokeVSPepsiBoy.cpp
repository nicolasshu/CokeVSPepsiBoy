//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/CollisionPolygon2D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Urho2D/TileMap2D.h>
#include <Urho3D/Urho2D/TileMapLayer2D.h>
#include <Urho3D/Urho2D/TmxFile2D.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>

#include <iostream>
#include <stdlib.h> // For sleep

#include <Urho3D/DebugNew.h>
#include <zconf.h>

#include "Character2D.h"
#include "Object2D.h"
#include "Mover.h"
#include "CokeVSPepsiBoy.h"

Urho2DIsometricDemo::Urho2DIsometricDemo(Context* context) :
    Sample(context),
    zoom_(2.0f),
    drawDebug_(false)
{
    // Register factory for the Character2D component so it can be created via CreateComponent
    Character2D::RegisterObject(context);
    // Register factory and attributes for the Mover component so it can be created via CreateComponent, and loaded / saved
    Mover::RegisterObject(context);
}

void Urho2DIsometricDemo::Setup()
{
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
}

void Urho2DIsometricDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    sample2D_ = new Sample2D(context_);

    // Set filename for load/save functions
    sample2D_->demoFilename_ = "Isometric2D";

    // Create the scene content
    CreateScene();

    // Create the UI content
    sample2D_->CreateUIContent("CokeVsPepsi", character2D_->remainingLifes_, character2D_->remainingPepsis_);
    auto* ui = GetSubsystem<UI>();
    Button* playButton = static_cast<Button*>(ui->GetRoot()->GetChild("PlayButton", true));
    SubscribeToEvent(playButton, E_RELEASED, URHO3D_HANDLER(Urho2DIsometricDemo, HandlePlayButton));

    // Hook up to the frame update events
    SubscribeToEvents();


}

void Urho2DIsometricDemo::CreateScene()
{
    scene_ = new Scene(context_);
    sample2D_->scene_ = scene_;

    // Create the Octree, DebugRenderer and PhysicsWorld2D components to the scene
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();
    auto* physicsWorld = scene_->CreateComponent<PhysicsWorld2D>();
    physicsWorld->SetGravity(Vector2(0.0f, 0.0f)); // Neutralize gravity as the character will always be grounded

    // Create camera
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    auto* graphics = GetSubsystem<Graphics>();
    camera->SetOrthoSize((float)graphics->GetHeight() * PIXEL_SIZE);
    camera->SetZoom(2.0f * Min((float)graphics->GetWidth() / 1280.0f, (float)graphics->GetHeight() / 800.0f)); // Set zoom according to user's resolution to ensure full visibility (initial zoom (2.0) is set for full visibility at 1280x800 resolution)
    cameraNode_->SetPosition(Vector3(-5.0f, 11.0f, -10.0f));

    // Setup the viewport for displaying the scene
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, camera));
    auto* renderer = GetSubsystem<Renderer>();
    renderer->SetViewport(0, viewport);

    auto* cache = GetSubsystem<ResourceCache>();

    // Create tile map from tmx file
    auto* tmxFile = cache->GetResource<TmxFile2D>("Urho2D/Tilesets/atrium.tmx");
    SharedPtr<Node> tileMapNode(scene_->CreateChild("TileMap"));
    auto* tileMap = tileMapNode->CreateComponent<TileMap2D>();
    tileMap->SetTmxFile(tmxFile);
    const TileMapInfo2D& info = tileMap->GetInfo();

    // Create Spriter Imp character (from sample 33_SpriterAnimation)
    Node* spriteNode = sample2D_->CreateCharacter(info, 0.0f, Vector3(-5.0f, 11.0f, 0.0f), 0.15f);
    character2D_ = spriteNode->CreateComponent<Character2D>(); // Create a logic component to handle character behavior
    // Scale character's speed on the Y axis according to tiles' aspect ratio
    character2D_->moveSpeedScale_ = info.tileHeight_ / info.tileWidth_;
    character2D_->zoom_ = camera->GetZoom();
    // Generate physics collision shapes from the tmx file's objects located in "Physics" (top) layer
    TileMapLayer2D* tileMapLayer = tileMap->GetLayer(tileMap->GetNumLayers() - 1);
    sample2D_->CreateCollisionShapesFromTMXObjects(tileMapNode, tileMapLayer, info);

    // Instantiate enemies at each placeholder of "MovingEntities" layer (placeholders are Poly Line objects defining a path from points)
    sample2D_->PopulateMovingEntities(tileMap->GetLayer(tileMap->GetNumLayers() - 2));

    // Instantiate pepsis to pick at each placeholder of "Pepsis" layer (placeholders for pepsis are Rectangle objects)
    TileMapLayer2D* pepsisLayer = tileMap->GetLayer(tileMap->GetNumLayers() - 3);
    sample2D_->PopulatePepsis(pepsisLayer);

    // Init pepsis counters
    character2D_->remainingPepsis_ = pepsisLayer->GetNumObjects();
    character2D_->maxPepsis_ = pepsisLayer->GetNumObjects();

    // Check when scene is rendered
    SubscribeToEvent(E_ENDRENDERING, URHO3D_HANDLER(Urho2DIsometricDemo, HandleSceneRendered));
}

void Urho2DIsometricDemo::HandleCollisionBegin(StringHash eventType, VariantMap& eventData) {

    // Get colliding node
    auto* hitNode = static_cast<Node*>(eventData[PhysicsBeginContact2D::P_NODEA].GetPtr());
    if (hitNode->GetName() == "Imp")
        hitNode = static_cast<Node*>(eventData[PhysicsBeginContact2D::P_NODEB].GetPtr());
    String nodeName = hitNode->GetName();
    Node* character2DNode = scene_->GetChild("Imp", true);

    // Handle Pepsis picking
    if (nodeName == "Pepsi") {

        hitNode->Remove();
        character2D_->remainingPepsis_ -= 1;
        auto* ui = GetSubsystem<UI>();
        if (character2D_->remainingPepsis_ == 0) {
            Text* instructions = static_cast<Text*>(ui->GetRoot()->GetChild("Instructions", true));
            instructions->SetText("You have all the Pepsi. Drink it all !!!");
            static_cast<Text*>(ui->GetRoot()->GetChild("ExitButton", true))->SetVisible(true);
            static_cast<Text*>(ui->GetRoot()->GetChild("PlayButton", true))->SetVisible(true);


            sample2D_->PlaySoundEffect("Winning.wav");
        }
        Text* pepsiText = static_cast<Text*>(ui->GetRoot()->GetChild("PepsiText", true));
        pepsiText->SetText(String(character2D_->remainingPepsis_)); // Update pepsis UI counter
        sample2D_->PlaySoundEffect("OpeningSoda.wav");
    }

    // Handle interactions with enemies
    if (nodeName == "Orc") {

        auto* animatedSprite = character2DNode->GetComponent<AnimatedSprite2D>();
        float deltaX = character2DNode->GetPosition().x_ - hitNode->GetPosition().x_;

            if (!character2DNode->GetChild("Emitter", true)) {

                character2D_->wounded_ = true;
                if (nodeName == "Orc") {
                    auto* brian = static_cast<Mover*>(hitNode->GetComponent<Mover>());
                    brian->fightTimer_ = 1;
                }
                sample2D_->SpawnEffect(character2DNode);
                sample2D_->PlaySoundEffect("BigExplosion.wav");
            }
    }


}

void Urho2DIsometricDemo::HandleSceneRendered(StringHash eventType, VariantMap& eventData) {

    UnsubscribeFromEvent(E_ENDRENDERING);
    // Save the scene so we can reload it later
    sample2D_->SaveScene(true);
    // Pause the scene as long as the UI is hiding it
    scene_->SetUpdateEnabled(false);
}

void Urho2DIsometricDemo::SubscribeToEvents() {

    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Urho2DIsometricDemo, HandleUpdate));

    // Subscribe HandlePostUpdate() function for processing post update events
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Urho2DIsometricDemo, HandlePostUpdate));

    // Subscribe to PostRenderUpdate to draw debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Urho2DIsometricDemo, HandlePostRenderUpdate));

    // Unsubscribe the SceneUpdate event from base class to prevent camera pitch and yaw in 2D sample
    UnsubscribeFromEvent(E_SCENEUPDATE);

    // Subscribe to Box2D contact listeners
    SubscribeToEvent(E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(Urho2DIsometricDemo, HandleCollisionBegin));

}

void Urho2DIsometricDemo::HandleUpdate(StringHash eventType, VariantMap& eventData) {

    using namespace Update;

    // Zoom in/out
    if (cameraNode_)
        sample2D_->Zoom(cameraNode_->GetComponent<Camera>());

    auto* input = GetSubsystem<Input>();

    // Toggle debug geometry with 'Z' key
    if (input->GetKeyPress(KEY_Z))
        drawDebug_ = !drawDebug_;

    // Check for loading / saving the scene
    if (input->GetKeyPress(KEY_F5))
        sample2D_->SaveScene(false);

    if (input->GetKeyPress(KEY_F7))
        ReloadScene(false);
}

void Urho2DIsometricDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData) {

    if (!character2D_)
        return;

    Node* character2DNode = character2D_->GetNode();
    float moveSpeedScale_(1.0f);
    auto* input = GetSubsystem<Input>();
    // Set direction
    Vector3 moveDir = Vector3::ZERO; // Reset
    float speedX = Clamp(MOVE_SPEED_X / zoom_, 0.4f, 1.0f);
    float speedY = speedX;

    if (input->GetKeyDown('A') ) {
        moveDir = moveDir + Vector3::LEFT * speedX;
    }
    if (input->GetKeyDown('D') ) {
        moveDir = moveDir + Vector3::RIGHT * speedX;
    }

    if (!moveDir.Equals(Vector3::ZERO))
        speedY = speedX * moveSpeedScale_;

    if (input->GetKeyDown('W') )
        moveDir = moveDir + Vector3::UP * speedY;

    if (input->GetKeyDown('S') )
        moveDir = moveDir + Vector3::DOWN * speedY;

    if (!moveDir.Equals(Vector3::ZERO)) {
        // node_->Translate(moveDir * timeStep);
        double timeStep = 0.005;
        moveDir = moveDir*timeStep;

        cameraNode_->SetPosition(Vector3(cameraNode_->GetPosition().x_+moveDir.x_, cameraNode_->GetPosition().y_+moveDir.y_,
                                         -10.0f)); // Camera tracks character
    }
    if (character2DNode->GetPosition().x_ == -5.0f && character2DNode->GetPosition().y_ == 11.0f) {
        cameraNode_->SetPosition(Vector3(-5.0f, 11.0f, -10.0f));
    }


    // std::cout << "Boy: (" << character2DNode->GetPosition().x_ << "," << character2DNode->GetPosition().y_ << "," << character2DNode->GetPosition().z_ << std::endl;
    // std::cout << "Camera Movement: (" << moveDir.x_ << ',' << moveDir.y_ << ',' << moveDir.z_ << ")" << std::endl;
    // std::cout << "Camera: (" << cameraNode_->GetPosition().x_ << ',' << cameraNode_->GetPosition().y_ << ',' << cameraNode_->GetPosition().z_ << ")" << std::endl;
}

void Urho2DIsometricDemo::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData) {

    if (drawDebug_) {
        auto* physicsWorld = scene_->GetComponent<PhysicsWorld2D>();
        physicsWorld->DrawDebugGeometry();

        Node* tileMapNode = scene_->GetChild("TileMap", true);
        auto* map = tileMapNode->GetComponent<TileMap2D>();
        map->DrawDebugGeometry(scene_->GetComponent<DebugRenderer>(), false);
    }
}

void Urho2DIsometricDemo::ReloadScene(bool reInit) {

    String filename = sample2D_->demoFilename_;
    if (!reInit)
        filename += "InGame";

    File loadFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/" + filename + ".xml", FILE_READ);
    scene_->LoadXML(loadFile);
    // After loading we have to reacquire the weak pointer to the Character2D component, as it has been recreated
    // Simply find the character's scene node by name as there's only one of them
    Node* character2DNode = scene_->GetChild("Imp", true);
    if (character2DNode)
        character2D_ = character2DNode->GetComponent<Character2D>();

    // Set what number to use depending whether reload is requested from 'PLAY' button (reInit=true) or 'F7' key (reInit=false)
    int lifes = character2D_->remainingLifes_;
    int pepsis = character2D_->remainingPepsis_;
    if (reInit) {
        lifes = LIFES;
        pepsis = character2D_->maxPepsis_;
    }

    // Update lifes UI
    auto* ui = GetSubsystem<UI>();
    Text* lifeText = static_cast<Text*>(ui->GetRoot()->GetChild("LifeText", true));
    lifeText->SetText(String(lifes));

    // Update pepsis UI
    Text* pepsiText = static_cast<Text*>(ui->GetRoot()->GetChild("PepsiText", true));
    pepsiText->SetText(String(pepsis));
}

void Urho2DIsometricDemo::HandlePlayButton(StringHash eventType, VariantMap& eventData) {

    // Remove fullscreen UI and unfreeze the scene
    auto* ui = GetSubsystem<UI>();
    if (static_cast<Text*>(ui->GetRoot()->GetChild("FullUI", true))) {
        ui->GetRoot()->GetChild("FullUI", true)->Remove();
        scene_->SetUpdateEnabled(true);
    }
    else
        // Reload scene
        ReloadScene(true);

    // Hide Instructions and Play/Exit buttons
    Text* instructionText = static_cast<Text*>(ui->GetRoot()->GetChild("Instructions", true));
    instructionText->SetText("");
    Button* exitButton = static_cast<Button*>(ui->GetRoot()->GetChild("ExitButton", true));
    exitButton->SetVisible(false);
    Button* playButton = static_cast<Button*>(ui->GetRoot()->GetChild("PlayButton", true));
    playButton->SetVisible(false);

    sample2D_->PlaySoundEffect("Yum.wav");

    // Hide mouse cursor
    auto* input = GetSubsystem<Input>();
    //input->SetMouseVisible(false);
    input->SetMouseVisible(true);
}

URHO3D_DEFINE_APPLICATION_MAIN(Urho2DIsometricDemo)
