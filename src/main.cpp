#include <Geode/modify/PlayerObject.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

static int maxFrames = 4;
auto chosenGameSprite = Mod::get()->getSettingValue<std::string>("selected-sprite");

$on_mod(Loaded) {
    listenForSettingChanges("selected-sprite", [](std::string value) {
        chosenGameSprite = value;
    });
}

class $modify(PlayerObject) {
    struct Fields {
        float m_animationTimer = 0.f;
        float m_bumpTimer = 0.f; 
        int m_maxFrames = 4; 
        int m_currentFrame = 1; 
        bool m_flippedX = false; 
        bool m_flippedY = false; 
        bool m_isUsingExtendedFrames = false;
        CCSprite* m_customSprite = nullptr;
    };

    bool init(int p0, int p1, GJBaseGameLayer* p2, cocos2d::CCLayer* p3, bool p4) {
        if (!PlayerObject::init(p0, p1, p2, p3, p4)) return false;

        auto fields = m_fields.self();

        // Change frames depending on what sprite u selected
        // Some sprites need to use 8 frames max
        // Some need only 4
        if (chosenGameSprite == "mania" || chosenGameSprite == "advance2" || chosenGameSprite == "supermania" || chosenGameSprite == "sonic2hd") {
            fields->m_maxFrames = 8;
            fields->m_isUsingExtendedFrames = true;
        } else {
            fields->m_maxFrames = 4;
            fields->m_isUsingExtendedFrames = false;
        }

        // Hide robot node
        m_robotBatchNode->setVisible(false);

        // give birth to sonic (real)
        std::string frameName = fmt::format("{}_sonicRun_01.png"_spr, chosenGameSprite);
        fields->m_customSprite = CCSprite::createWithSpriteFrameName(frameName.c_str());
        if (fields->m_customSprite) {
            fields->m_customSprite->setAnchorPoint({0.5f, 0.5f});
            fields->m_customSprite->setPosition(this->getPosition());
            fields->m_customSprite->setVisible(false);
            fields->m_customSprite->setID("sonic-anim"_spr);
            this->addChild(fields->m_customSprite, 10);
        }

        return true;
    }

    void update(float p0) {
        PlayerObject::update(p0);

        auto fields = m_fields.self();

        // Sync rotation
        if (fields->m_customSprite && m_mainLayer) {
            fields->m_customSprite->setRotation(m_mainLayer->getRotation());

            // y flip
            bool mainLayerFlippedY = m_mainLayer->getScaleY() < 0;
            fields->m_customSprite->setFlipY(mainLayerFlippedY);
        }

        // Check if ur a robot
        if (!m_isRobot || !fields->m_customSprite) {
            if (fields->m_customSprite) {
                fields->m_customSprite->setVisible(false);
            }
            return;
        }

        // It looks like shit with the fire and particles
        // so fuck off
        m_robotFire->setVisible(false);
        m_robotBurstParticles->setVisible(false);

        fields->m_customSprite->setVisible(true);

        // bump anim for pads
        if (fields->m_bumpTimer > 0.f) {

            std::string frameName = fmt::format("{}_sonicBumped_01.png"_spr, chosenGameSprite);
            fields->m_customSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
            fields->m_bumpTimer -= 0.2f;
            fields->m_animationTimer = 0; 
            return;
        }

        // idle anim for platformer
        // the whole catalyst for this mod
        if (m_isPlatformer && m_platformerXVelocity == 0 && m_isOnGround) {

            std::string frameName = fmt::format("{}_sonicIdle_01.png"_spr, chosenGameSprite);
            fields->m_customSprite->setDisplayFrame(CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str()));
            fields->m_animationTimer = 0;
        } else {
            // set current animation (run or jump)
            std::string frameName;
            float frameDuration;

            if (this->m_isOnGround || this->m_hasGroundParticles) {
                frameName = fmt::format("{}_sonicRun_0{}.png"_spr, chosenGameSprite, fields->m_currentFrame);
                // change frame times if using extended (8) frames
                if (fields->m_isUsingExtendedFrames){
                    frameDuration = 1.9f;
                } else {
                    frameDuration = 2.1f;
                }
            } else {
                frameName = fmt::format("{}_sonicJump_0{}.png"_spr, chosenGameSprite, fields->m_currentFrame);
                // change frame times if using extended (8) frames
                if (fields->m_isUsingExtendedFrames){
                    frameDuration = 0.7f;
                } else {
                    frameDuration = 1.0f;
                }
            }

            // update animation frame
            fields->m_animationTimer += p0;

            if (fields->m_animationTimer >= frameDuration && fields->m_bumpTimer <= 0.1f) {
                fields->m_animationTimer -= frameDuration;

                // update current frame (cycle through 1-4/8)
                fields->m_currentFrame = (fields->m_currentFrame % fields->m_maxFrames) + 1;
                auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());

                // Only update if valid
                if (frame) {
                    fields->m_customSprite->setDisplayFrame(frame);
                }
            }
        }

        // if i need to add anything else
        // do it here

    }

    void bumpPlayer(float p0, int p1, bool p2, GameObject* p3) {
        PlayerObject::bumpPlayer(p0, p1, p2, p3);

        auto fields = m_fields.self();

        if (m_isRobot && fields->m_customSprite) {
            fields->m_bumpTimer = 12.5f; 
        }
        
    }

    void playerDestroyed(bool p0) {
        PlayerObject::playerDestroyed(p0);

        m_fields->m_customSprite->setVisible(false);
    }

//    virtual void setFlipX(bool p0) override {
//
//        auto fields = m_fields.self();
//
//        if (p0 != fields->m_flippedX) {
//            fields->m_flippedX = p0;
//            fields->m_customSprite->setFlipX(p0); 
//        }
//
//        PlayerObject::setFlipX(p0);
//    }

    void doReversePlayer(bool p0) {

        auto fields = m_fields.self();

        if (p0 != fields->m_flippedX) {
            fields->m_flippedX = p0;
            fields->m_customSprite->setFlipX(p0); 
        }

        PlayerObject::doReversePlayer(p0);
    }

    void setVisible(bool visible) {
        PlayerObject::setVisible(visible);

        auto fields = m_fields.self();

        if (fields->m_customSprite) {
            fields->m_customSprite->setVisible(visible);
        }
    }
};