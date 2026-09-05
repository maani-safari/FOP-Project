#include "coach.h"
#include "core/constants.h"
#include "entities/ball.h"
#include "entities/team.h"
#include "game/scene.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// Set to false to let the other team use their own logic (if you implement it)
// Set to true to test your logic on both teams
bool coach_both_teams = true;

/* -------------------------------------------------------------------------
 * Logic Functions
 *  TODO 1: You must implement the following functions in Phase 2.
 *        Each player in each team has its own functions.
 *        You can add new functions, but are NOT ALLOWED to remove
 *        the existing functions or change their structure.
 * -------------------------------------------------------------------------
 * ⚠️ STUDENT RULES FOR PHASE 2:
 * You are restricted to modifying ONLY specific variables in each function:
 *
 * 1. MOVEMENT FUNCTIONS (movement_logic_X_Y):
 * Allowed: player->velocity
 * Goal:    Determine the direction and speed of movement.
 *
 * 2. SHOOTING FUNCTIONS (shooting_logic_X_Y):
 * Allowed: ball->velocity
 * Goal:    Determine the direction and power of the kick/pass.
 *
 * 3. CHANGE STATE FUNCTIONS (change_state_logic_X_Y):
 * Allowed: player->state
 * Goal:    Switch between IDLE, MOVING, SHOOTING, or INTERCEPTING.
 *
 * NOTE: Directly modifying any other attributes will be flagged as a violation.
 * Thank you for your attention to this matter!
 * ------------------------------------------------------------------------- */

/* Team 1 movement logic */
static void check_out_player(struct Player *self)
{
    if (self->position.x > PITCH_W + PITCH_X + PLAYER_RADIUS)
    {
        if (self->velocity.x > 0)
        {
            self->velocity.x = -(self->velocity.x);
        }
    }
    if (self->position.x < PITCH_X - PLAYER_RADIUS)
    {
        if (self->velocity.x < 0)
        {
            self->velocity.x = -(self->velocity.x);
        }
    }
    if (self->position.y < PITCH_Y - PLAYER_RADIUS)
    {
        if (self->velocity.y < 0)
        {
            self->velocity.y = -(self->velocity.y);
        }
    }
    if (self->position.y > PITCH_H + PITCH_Y + PLAYER_RADIUS)
    {
        if (self->velocity.y > 0)
        {
            self->velocity.y = -(self->velocity.y);
        }
    }
}
static void goal_keeper_place(struct Player *self, struct Scene *scene)
{
    int box_w = 80;
    int box_h = 200;
    float box_left, box_right;
    float box_top = CENTER_Y - (box_h / 2);
    float box_bottom = CENTER_Y + (box_h / 2);

    if (self->team == 1)
    {
        box_left = PITCH_X;
        box_right = PITCH_X + box_w;
    }
    else
    {
        box_left = PITCH_X + PITCH_W - box_w;
        box_right = PITCH_X + PITCH_W;
    }

    float target_x, target_y;
    float ball_x = scene->ball->position.x;
    float ball_y = scene->ball->position.y;

    target_x = (box_left + box_right) / 2;
    target_y = CENTER_Y;

    bool ball_in_our_third_half = (self->team == 1) ? (ball_x < CENTER_X) : (ball_x > CENTER_X);

    if (ball_in_our_third_half)
    {

        if (self->team == 1)
            target_x = fmaxf(box_left, fminf(box_right - 10, ball_x + 20));
        else
            target_x = fmaxf(box_left + 10, fminf(box_right, ball_x - 20));

        target_y = ball_y;
        target_y = fmaxf(box_top + 10, fminf(box_bottom - 10, target_y));
    }

    float dx = target_x - self->position.x;
    float dy = target_y - self->position.y;
    float dist = hypotf(dx, dy);

    float max_speed = MAX_PLAYER_VELOCITY * ((float)self->talents.agility / MAX_TALENT_PER_SKILL);
    float speed = max_speed * 0.9f;

    if (dist < 5.0f)
    {
        self->velocity.x = 0.0f;
        self->velocity.y = 0.0f;
    }
    else
    {
        self->velocity.x = (dx / dist) * speed;
        self->velocity.y = (dy / dist) * speed;
    }
}
static float clampf_local(float v, float min_v, float max_v)
{
    if (v < min_v)
        return min_v;
    if (v > max_v)
        return max_v;
    return v;
}
static bool is_closest_to_ball(struct Player *self, struct Scene *scene)
{
    struct Team *my_team = (self->team == 1) ? scene->first_team : scene->second_team;
    float my_dist = hypotf(scene->ball->position.x - self->position.x,
                           scene->ball->position.y - self->position.y);

    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        struct Player *other = my_team->players[i];
        if (!other || other == self)
            continue;

        float other_dist = hypotf(scene->ball->position.x - other->position.x,
                                  scene->ball->position.y - other->position.y);
        if (other_dist < my_dist - 15.0f)
        {
            return false;
        }
    }
    return true;
}

static void move_to_target(struct Player *self, float target_x, float target_y, float speed_scale)
{
    float dx = target_x - self->position.x;
    float dy = target_y - self->position.y;
    float dist = hypotf(dx, dy);
    float max_speed = MAX_PLAYER_VELOCITY * ((float)self->talents.agility / MAX_TALENT_PER_SKILL);
    float speed = max_speed * speed_scale;

    if (dist < 4.0f)
    {
        self->velocity.x = 0.0f;
        self->velocity.y = 0.0f;
        return;
    }

    self->velocity.x = (dx / dist) * speed;
    self->velocity.y = (dy / dist) * speed;
}
static void attacking_movement(struct Player *self, struct Scene *scene)
{
    float bx = scene->ball->position.x;
    float by = scene->ball->position.y;

    bool is_defender = (self->kit == 2 || self->kit == 4);

    float max_forward_x;
    if (self->team == 1)
    {
        max_forward_x = (is_defender) ? CENTER_X - 50 : PITCH_X + PITCH_W - 50;
    }
    else
    {
        max_forward_x = (is_defender) ? CENTER_X + 50 : PITCH_X + 50;
    }

    float direction = (self->team == 1) ? 1.0f : -1.0f;

    float target_x, target_y;

    switch (self->kit)
    {
    case 0:
        target_x = bx + (120.0f * direction);
        target_y = by;
        break;
    case 1:
    case 5:
        target_x = bx + (100.0f * direction);
        target_y = by + ((self->kit == 1) ? -70.0f : 70.0f);
        break;
    case 2:
    case 4:
        target_x = bx + (40.0f * direction);
        target_y = by + ((self->kit == 2) ? -30.0f : 30.0f);
        break;
    default:
        float error = (rand() % 20 - 10);
        target_x = bx + (60.0f * direction) + error;
        target_y = by;
        break;
    }

    if (is_defender)
    {
        if (self->team == 1)
        {
            target_x = clampf_local(target_x, PITCH_X + 30, max_forward_x);
        }
        else
        {
            target_x = clampf_local(target_x, max_forward_x, PITCH_X + PITCH_W - 30);
        }
    }
    else
    {
        target_x = clampf_local(target_x, PITCH_X + 30, PITCH_X + PITCH_W - 30);
    }

    target_y = clampf_local(target_y, PITCH_Y + 40, PITCH_Y + PITCH_H - 40);

    float dist_to_ball = hypotf(bx - self->position.x, by - self->position.y);
    float speed_scale = (is_defender) ? 0.7f : 0.85f;
    if (dist_to_ball > 200.0f)
        speed_scale += 0.15f;

    move_to_target(self, target_x, target_y, speed_scale);
}
static void move_field_player(struct Player *self, struct Scene *scene)
{
    struct Vec2 home = get_positions(self->team, self->kit);
    float bx = scene->ball->position.x;
    float by = scene->ball->position.y;

    if (self == scene->ball->possessor)
    {
        float goal_x = (self->team == 1) ? (PITCH_X + PITCH_W) : PITCH_X;
        float target_y = CENTER_Y;

        struct Team *opp_team = (self->team == 1) ? scene->second_team : scene->first_team;
        float attack_dir = (self->team == 1) ? 1.0f : -1.0f;

        bool blocked = false;
        float min_dist_to_blocker = 100.0f;

        for (int i = 0; i < PLAYER_COUNT; i++)
        {
            struct Player *opp = opp_team->players[i];

            if (!opp || opp->kit == 3)
                continue;

            float dx = (opp->position.x - self->position.x) * attack_dir;
            float dy = fabsf(opp->position.y - self->position.y);

            if (dx > 0 && dx < 85.0f && dy < 50.0f)
            {
                blocked = true;

                target_y = self->position.y + (opp->position.y > self->position.y ? -60.0f : 60.0f);
                break;
            }
        }

        if (!blocked)
        {
            target_y = CENTER_Y + (sinf(self->position.x * 0.04f) * 20.0f);
        }

        move_to_target(self, goal_x, target_y, 1.0f);
        return;
    }
    bool is_forward = (self->kit == 0 || self->kit == 1 || self->kit == 5);
    bool is_defender = (self->kit == 2 || self->kit == 4);
    bool is_midfielder = (self->kit == 1 || self->kit == 5);

    if (scene->ball->possessor != NULL && scene->ball->possessor->team == self->team)
    {
        if (is_forward)
        {
            attacking_movement(self, scene);
            return;
        }

        if (is_defender)
        {

            float max_forward = (self->team == 1) ? CENTER_X : CENTER_X;
            bool ball_in_our_half = (self->team == 1) ? (bx < CENTER_X + 100) : (bx > CENTER_X - 100);

            if (!ball_in_our_half)
            {

                float defensive_x = (self->team == 1) ? CENTER_X - 100 : CENTER_X + 100;
                move_to_target(self, defensive_x, home.y, 0.6f);
            }
            else
            {

                float support_x = bx + (self->team == 1 ? -60.0f : 60.0f);
                support_x = clampf_local(support_x,
                                         (self->team == 1) ? PITCH_X + 50 : CENTER_X,
                                         (self->team == 1) ? CENTER_X : PITCH_X + PITCH_W - 50);
                move_to_target(self, support_x, home.y, 0.7f);
            }
            return;
        }

        if (is_midfielder)
        {
            attacking_movement(self, scene);
            return;
        }

        move_to_target(self, home.x, home.y, 0.8f);
        return;
    }

    if (scene->ball->possessor != NULL && scene->ball->possessor->team != self->team)
    {
        struct Player *opp_possessor = scene->ball->possessor;
        float dist_to_opp = hypotf(opp_possessor->position.x - self->position.x,
                                   opp_possessor->position.y - self->position.y);

        if (self->kit == 2 || self->kit == 4)
        {
            float our_goal_x = (self->team == 1) ? PITCH_X : PITCH_X + PITCH_W;
            float dist_opp_to_goal = fabsf(opp_possessor->position.x - our_goal_x);
            bool in_defensive_third = (dist_opp_to_goal < PITCH_W * 0.35f);

            bool near_goal = (dist_opp_to_goal < 150.0f);

            bool i_am_closest_defender = is_closest_to_ball(self, scene);

            if (near_goal && i_am_closest_defender && dist_to_opp < 120.0f)
            {

                move_to_target(self, opp_possessor->position.x,
                               opp_possessor->position.y, 0.85f);
                return;
            }
            else if (in_defensive_third)
            {

                float def_x = (self->team == 1) ? our_goal_x + 60.0f + (dist_opp_to_goal * 0.2f) : our_goal_x - 60.0f - (dist_opp_to_goal * 0.2f);

                if (self->team == 1)
                    def_x = fminf(def_x, CENTER_X - 30.0f);
                else
                    def_x = fmaxf(def_x, CENTER_X + 30.0f);

                move_to_target(self, def_x,
                               opp_possessor->position.y * 0.5f + CENTER_Y * 0.5f, 0.65f);
                return;
            }
            else
            {

                struct Vec2 home = get_positions(self->team, self->kit);
                move_to_target(self, home.x, home.y, 0.6f);
                return;
            }
        }

        bool i_am_closest = is_closest_to_ball(self, scene);

        if (i_am_closest && dist_to_opp < 250.0f)
        {
            move_to_target(self, opp_possessor->position.x, opp_possessor->position.y, 1.0f);
            return;
        }
        else
        {
            struct Vec2 home = get_positions(self->team, self->kit);
            float fallback_x = (self->team == 1) ? fmaxf(PITCH_X + 80, home.x - 40) : fminf(PITCH_X + PITCH_W - 80, home.x + 40);
            move_to_target(self, fallback_x, home.y, 0.8f);
            return;
        }
    }

    if (is_closest_to_ball(self, scene))
    {
        move_to_target(self, bx, by, 1.0f);
        return;
    }

    move_to_target(self, home.x, home.y, 0.8f);
}
static void clamp_player_velocity(struct Player *self)
{
    float max_speed = MAX_PLAYER_VELOCITY * ((float)self->talents.agility / MAX_TALENT_PER_SKILL);

    if (fabsf(self->velocity.x) > max_speed)
    {
        self->velocity.x = (self->velocity.x > 0) ? max_speed : -max_speed;
    }

    if (fabsf(self->velocity.y) > max_speed)
    {
        self->velocity.y = (self->velocity.y > 0) ? max_speed : -max_speed;
    }
}
void check_collisions(struct Player *self, struct Scene *scene)
{
    struct Ball *ball = scene->ball;
    struct Team *team1 = scene->first_team;
    struct Team *team2 = scene->second_team;

    float dx = self->position.x - ball->position.x;
    float dy = self->position.y - ball->position.y;
    float dist = hypotf(dx, dy);
    float minDist = PLAYER_RADIUS + BALL_RADIUS;

    if (dist < minDist)
    {

        float nx = dx / dist;
        float ny = dy / dist;

        float vrel_x = self->velocity.x - ball->velocity.x;
        float vrel_y = self->velocity.y - ball->velocity.y;
        float vrel_n = vrel_x * nx + vrel_y * ny;

        if (vrel_n < 0)
        {

            float e = 0.7f;

            float mass_ratio = 0.2f;
            float impulse = (1.0f + e) * vrel_n / (1.0f + mass_ratio);

            ball->velocity.x -= impulse * nx;
            ball->velocity.y -= impulse * ny;

            self->velocity.x += mass_ratio * impulse * nx;
            self->velocity.y += mass_ratio * impulse * ny;

            float overlap = minDist - dist;
            float sep_x = nx * overlap * 0.5f;
            float sep_y = ny * overlap * 0.5f;
            self->position.x += sep_x;
            self->position.y += sep_y;
            ball->position.x -= sep_x;
            ball->position.y -= sep_y;
        }
    }

    struct Player *players[12];
    int count = 0;
    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (team1->players[i] && team1->players[i] != self)
            players[count++] = team1->players[i];
        if (team2->players[i] && team2->players[i] != self)
            players[count++] = team2->players[i];
    }

    for (int i = 0; i < count; i++)
    {
        struct Player *other = players[i];
        float dx = self->position.x - other->position.x;
        float dy = self->position.y - other->position.y;
        float dist = hypotf(dx, dy);
        float minDist = PLAYER_RADIUS + PLAYER_RADIUS;

        if (dist < minDist)
        {
            float nx = dx / dist;
            float ny = dy / dist;

            float vrel_x = self->velocity.x - other->velocity.x;
            float vrel_y = self->velocity.y - other->velocity.y;
            float vrel_n = vrel_x * nx + vrel_y * ny;

            if (vrel_n < 0)
            {

                float e = 0.5f;
                float impulse = (1.0f + e) * vrel_n * 0.5f;

                self->velocity.x -= impulse * nx;
                self->velocity.y -= impulse * ny;
                other->velocity.x += impulse * nx;
                other->velocity.y += impulse * ny;

                float overlap = minDist - dist;
                float sep_x = nx * overlap * 0.5f;
                float sep_y = ny * overlap * 0.5f;
                self->position.x += sep_x;
                self->position.y += sep_y;
                other->position.x -= sep_x;
                other->position.y -= sep_y;
            }
        }
    }
}

void movement_logic_1_0(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_1_1(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_1_2(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_1_3(struct Player *self, struct Scene *scene)
{
    goal_keeper_place(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_1_4(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_1_5(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}

/* Team 2 movement logic */
void movement_logic_2_0(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_2_1(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_2_2(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_2_3(struct Player *self, struct Scene *scene)
{
    goal_keeper_place(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_2_4(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void movement_logic_2_5(struct Player *self, struct Scene *scene)
{
    move_field_player(self, scene);
    check_out_player(self);
    check_collisions(self, scene);
    clamp_player_velocity(self);
}
void limit_shooting(struct Player *self, struct Scene *scene)
{
    float limit = MAX_BALL_VELOCITY * ((float)self->talents.shooting / MAX_TALENT_PER_SKILL);

    if (fabsf(scene->ball->velocity.x) > limit)
    {
        scene->ball->velocity.x = (scene->ball->velocity.x > 0) ? limit : -limit;
    }

    if (fabsf(scene->ball->velocity.y) > limit)
    {
        scene->ball->velocity.y = (scene->ball->velocity.y > 0) ? limit : -limit;
    }
}
void limit_kickoff(struct Player *self, struct Scene *scene)
{
    float distance_x = CENTER_X - scene->ball->position.x;
    float distance_y = CENTER_Y - scene->ball->position.y;
    if (fabs(distance_x) < 20.0f && fabs(distance_y) < 20.0f)
    {
        if (self->team == 1 && scene->ball->velocity.x > 0)
            scene->ball->velocity.x = -(scene->ball->velocity.x);

        if (self->team == 2 && scene->ball->velocity.x < 0)
            scene->ball->velocity.x = -(scene->ball->velocity.x);
    }
}
void limit_out(struct Player *self, struct Scene *scene)
{
    if (scene->state == STATE_RESTARTING || scene->state == STATE_OUT)
    {
        if (fabs(scene->ball->position.x - scene->ball->radius - PITCH_X) < 2.0f && (fabs(scene->ball->position.y - scene->ball->radius - PITCH_Y) < 2.0f))
        {
            if (scene->ball->velocity.x < 0)
            {
                scene->ball->velocity.x = -(scene->ball->velocity.x);
            }
            if (scene->ball->velocity.y < 0)
            {
                scene->ball->velocity.y = -(scene->ball->velocity.y);
            }
        }
        if (fabs(scene->ball->position.x - scene->ball->radius - PITCH_X) < 2.0f && (fabs(scene->ball->position.y + scene->ball->radius - (PITCH_Y + PITCH_H)) < 2.0f))
        {
            if (scene->ball->velocity.x < 0)
            {
                scene->ball->velocity.x = -(scene->ball->velocity.x);
            }
            if (scene->ball->velocity.y > 0)
            {
                scene->ball->velocity.y = -(scene->ball->velocity.y);
            }
        }
        if (fabs(scene->ball->position.x + scene->ball->radius - PITCH_X - PITCH_W) < 2.0f && (fabs(scene->ball->position.y - scene->ball->radius - PITCH_Y) < 2.0f))
        {
            if (scene->ball->velocity.x > 0)
            {
                scene->ball->velocity.x = -(scene->ball->velocity.x);
            }
            if (scene->ball->velocity.y < 0)
            {
                scene->ball->velocity.y = -(scene->ball->velocity.y);
            }
        }
        if (fabs(scene->ball->position.x + scene->ball->radius - PITCH_X - PITCH_W) < 2.0f && (fabs(scene->ball->position.y + scene->ball->radius - (PITCH_Y + PITCH_H)) < 2.0f))
        {
            if (scene->ball->velocity.x > 0)
            {
                scene->ball->velocity.x = -(scene->ball->velocity.x);
            }
            if (scene->ball->velocity.y > 0)
            {
                scene->ball->velocity.y = -(scene->ball->velocity.y);
            }
        }
        if ((fabs(scene->ball->position.y - scene->ball->radius - PITCH_Y) < 2.0f) && (scene->ball->velocity.y < 0))
        {
            scene->ball->velocity.y = -(scene->ball->velocity.y);
        }
        if ((fabs(scene->ball->position.y + scene->ball->radius - (PITCH_Y + PITCH_H)) < 2.0f) && (scene->ball->velocity.y > 0))
        {
            scene->ball->velocity.y = -(scene->ball->velocity.y);
        }
        if (fabs(scene->ball->position.x - scene->ball->radius - PITCH_X) < 2.0f)
        {
            if (scene->ball->velocity.x < 0)
            {
                scene->ball->velocity.x = -(scene->ball->velocity.x);
            }
        }
        if (fabs(scene->ball->position.x + scene->ball->radius - PITCH_X - PITCH_W) < 2.0f)
        {
            if (scene->ball->velocity.x > 0)
            {
                scene->ball->velocity.x = -(scene->ball->velocity.x);
            }
        }
    }
}
static void prevent_own_goal_shot(struct Player *self, struct Scene *scene)
{
    struct Ball *ball = scene->ball;
    float defensive_line = PITCH_X + (PITCH_W * 0.35f);
    bool toward_own_goal = (self->team == 1) ? (ball->velocity.x < 0.0f) : (ball->velocity.x > 0.0f);
    bool in_defensive_third = (self->team == 1) ? (ball->position.x < defensive_line)
                                                : (ball->position.x > (PITCH_X + PITCH_W - (PITCH_W * 0.35f)));

    if (scene->state == STATE_RESTARTING)
        return;
    if (toward_own_goal && in_defensive_third)
    {
        float speed = 100.0f + (float)(rand() % 100);
        float angle = ((float)rand() / (float)RAND_MAX) * 1.4f - 0.7f;
        if (self->team == 1)
        {
            ball->velocity.x = cosf(angle) * speed;
            ball->velocity.y = sinf(angle) * speed;
        }
        else
        {
            ball->velocity.x = -cosf(angle) * speed;
            ball->velocity.y = sinf(angle) * speed;
        }
    }
}
static int count_near_opponents(struct Scene *scene, struct Vec2 pos, int team, float radius)
{
    int count = 0;
    struct Team *opp = (team == 1) ? scene->second_team : scene->first_team;
    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (!opp->players[i])
            continue;
        float dx = opp->players[i]->position.x - pos.x;
        float dy = opp->players[i]->position.y - pos.y;
        if (hypotf(dx, dy) < radius)
            count++;
    }
    return count;
}
static void shoot_to_corners(struct Player *self, struct Scene *scene)
{
    struct Ball *ball = scene->ball;
    struct Team *opponent_team = (self->team == 1) ? scene->second_team : scene->first_team;
    struct Player *goalkeeper = opponent_team->players[3];

    float goal_top = CENTER_Y - (GOAL_HEIGHT / 2.0f) + BALL_RADIUS + 15.0f;
    float goal_bottom = CENTER_Y + (GOAL_HEIGHT / 2.0f) - BALL_RADIUS - 15.0f;
    float goal_upper_mid = CENTER_Y - (GOAL_HEIGHT / 4.0f);
    float goal_lower_mid = CENTER_Y + (GOAL_HEIGHT / 4.0f);

    float targets_y[5] = {
        goal_top,
        goal_bottom,
        goal_upper_mid,
        goal_lower_mid,
        CENTER_Y};

    int priority[5] = {0, 1, 2, 3, 4};

    if (goalkeeper && goalkeeper->state != STATE_OUT)
    {

        if (goalkeeper->position.y < CENTER_Y - 40.0f)
        {

            priority[0] = 1;
            priority[1] = 3;
            priority[2] = 4;
            priority[3] = 0;
            priority[4] = 2;
        }

        else if (goalkeeper->position.y > CENTER_Y + 40.0f)
        {
            priority[0] = 0;
            priority[1] = 2;
            priority[2] = 4;
            priority[3] = 1;
            priority[4] = 3;
        }

        else
        {
            priority[0] = 0;
            priority[1] = 1;
            priority[2] = 2;
            priority[3] = 3;
            priority[4] = 4;
        }
    }

    int selected_index;
    float shooting_power = (float)self->talents.shooting / MAX_TALENT_PER_SKILL;

    if (shooting_power > 0.8f)
    {

        selected_index = priority[0];
    }
    else if (shooting_power > 0.5f)
    {

        selected_index = (rand() % 100 < 70) ? priority[0] : priority[1];
    }
    else
    {

        int r = rand() % 3;
        selected_index = priority[r];
    }

    float target_y = targets_y[selected_index];
    float goal_x = (self->team == 1) ? (PITCH_X + PITCH_W + GOAL_WIDTH) : (PITCH_X - GOAL_WIDTH);

    float dx = goal_x - ball->position.x;
    float dy = target_y - ball->position.y;
    float distance = hypotf(dx, dy);
    float max_power = MAX_BALL_VELOCITY * shooting_power;
    float power = max_power;

    if (distance < 150.0f)
    {
        power = max_power * 0.6f;
    }

    if (distance < 1.0f)
    {
        ball->velocity.x = (self->team == 1) ? power : -power;
        ball->velocity.y = 0.0f;
        return;
    }

    ball->velocity.x = (dx / distance) * power;
    ball->velocity.y = (dy / distance) * power;

    float accuracy = shooting_power;
    float noise = (1.0f - accuracy) * 30.0f;
    ball->velocity.x += ((float)rand() / RAND_MAX - 0.5f) * noise;
    ball->velocity.y += ((float)rand() / RAND_MAX - 0.5f) * noise;
}
static void pass_intelligent(struct Player *self, struct Scene *scene)
{
    struct Team *my_team = (self->team == 1) ? scene->first_team : scene->second_team;
    struct Team *opp_team = (self->team == 1) ? scene->second_team : scene->first_team;

    float goal_x = (self->team == 1) ? (PITCH_X + PITCH_W) : PITCH_X;
    float distance_to_goal = fabs(goal_x - scene->ball->position.x);
    bool in_dangerous_position = (distance_to_goal < 200.0f);

    int best_target = -1;
    float best_score = 1e12f;
    float MIN_PASS_DISTANCE = (in_dangerous_position) ? 30.0f : 50.0f;

    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        struct Player *target = my_team->players[i];
        if (!target || target == self)
            continue;
        if (target->kit == 3)
            continue;

        float dx = target->position.x - self->position.x;
        float dy = target->position.y - self->position.y;
        float distance = hypotf(dx, dy);

        if (distance < MIN_PASS_DISTANCE)
        {
            continue;
        }

        int opponents_near = count_near_opponents(scene, target->position, self->team, 50.0f);

        // تعریف moving_forward داخل حلقه
        bool moving_forward = (self->team == 1) ? (target->position.x > self->position.x) : (target->position.x < self->position.x);

        float score = 0.0f;
        score += opponents_near * 150.0f;

        if (!moving_forward)
        {
            if (in_dangerous_position)
            {
                score += 2000.0f;
            }
            else
            {
                score += 400.0f;
            }
        }
        else
        {
            score -= distance * 0.5f;
        }

        if (in_dangerous_position && moving_forward && opponents_near == 0)
        {
            score -= 100.0f;
        }

        if (score < best_score)
        {
            best_score = score;
            best_target = i;
        }
    }

    if (best_target == -1)
    {
        if (in_dangerous_position)
        {
            shoot_to_corners(self, scene);
            return;
        }
        return;
    }

    struct Player *receiver = my_team->players[best_target];

    bool best_moving_forward = (self->team == 1) ? (receiver->position.x > self->position.x) : (receiver->position.x < self->position.x);

    float pass_x = receiver->position.x;
    float pass_y = receiver->position.y;

    float dx = pass_x - scene->ball->position.x;
    float dy = pass_y - scene->ball->position.y;
    float distance = hypotf(dx, dy);

    float max_power = MAX_BALL_VELOCITY * ((float)self->talents.shooting / MAX_TALENT_PER_SKILL);
    float min_power = 180.0f;

    if (in_dangerous_position)
    {
        min_power = 250.0f;
    }

    float power = min_power + (max_power - min_power) * (distance / 450.0f);
    if (power > max_power)
        power = max_power;
    if (power < min_power)
        power = min_power;

    if (distance < 1.0f)
    {
        shoot_to_corners(self, scene);
        return;
    }

    scene->ball->velocity.x = (dx / distance) * power;
    scene->ball->velocity.y = (dy / distance) * power;

    float accuracy = (float)self->talents.shooting / MAX_TALENT_PER_SKILL;
    if (accuracy < 0.7f)
    {
        float error = (1.0f - accuracy) * 40.0f;
        scene->ball->velocity.x += ((float)rand() / RAND_MAX - 0.5f) * error;
        scene->ball->velocity.y += ((float)rand() / RAND_MAX - 0.5f) * error;
    }

    if (in_dangerous_position && !best_moving_forward)
    {

        shoot_to_corners(self, scene);
        return;
    }
}
static void basic_shoot(struct Player *self, struct Scene *scene)
{
    struct Ball *ball = scene->ball;
    float goal_x = (self->team == 1) ? (PITCH_X + PITCH_W) : PITCH_X;
    float distance_to_goal = fabs(goal_x - ball->position.x);

    struct Team *opp_team = (self->team == 1) ? scene->second_team : scene->first_team;
    struct Player *goalkeeper = opp_team->players[3];

    bool is_one_on_one = false;
    if (goalkeeper)
    {
        float dx_to_gk = fabs(goalkeeper->position.x - ball->position.x);
        float dy_to_gk = fabs(goalkeeper->position.y - ball->position.y);

        if (dx_to_gk < 140.0f && dy_to_gk < 100.0f)
        {

            int defenders_near = 0;
            for (int i = 0; i < PLAYER_COUNT; i++)
            {
                struct Player *opp = opp_team->players[i];
                if (!opp || opp->kit == 3)
                    continue;

                float dx = fabs(opp->position.x - ball->position.x);
                float dy = fabs(opp->position.y - ball->position.y);

                if (dx < 45.0f && dy < 35.0f)
                {
                    defenders_near++;
                }
            }

            if (defenders_near == 0)
            {
                is_one_on_one = true;
            }
        }
    }

    if (is_one_on_one)
    {

        if (goalkeeper)
        {
            float gk_y = goalkeeper->position.y;
            float goal_top = CENTER_Y - (GOAL_HEIGHT / 2.0f) + BALL_RADIUS + 20.0f;
            float goal_bottom = CENTER_Y + (GOAL_HEIGHT / 2.0f) - BALL_RADIUS - 20.0f;

            float target_y;
            if (gk_y > CENTER_Y)
            {
                target_y = goal_top + 15.0f;
            }
            else
            {
                target_y = goal_bottom - 15.0f;
            }

            float dx = goal_x - ball->position.x;
            float dy = target_y - ball->position.y;
            float distance = hypotf(dx, dy);
            float max_power = MAX_BALL_VELOCITY * ((float)self->talents.shooting / MAX_TALENT_PER_SKILL);

            if (distance > 1.0f)
            {
                ball->velocity.x = (dx / distance) * max_power;
                ball->velocity.y = (dy / distance) * max_power;
            }
        }
        else
        {

            shoot_to_corners(self, scene);
        }
        return;
    }

    int density = count_near_opponents(scene, ball->position, self->team, 65.0f);

    bool should_shoot = false;

    if (distance_to_goal < 150.0f)
    {
        should_shoot = true;
    }
    else if (distance_to_goal < 220.0f && density >= 1)
    {
        should_shoot = true;
    }
    else if (distance_to_goal < 300.0f && density == 0)
    {
        should_shoot = (rand() % 100 < 30);
    }

    if (should_shoot)
    {
        shoot_to_corners(self, scene);
    }
    else
    {
        pass_intelligent(self, scene);

        if (fabs(scene->ball->velocity.x) < 2.0f && fabs(scene->ball->velocity.y) < 2.0f)
        {
            shoot_to_corners(self, scene);
        }
    }
}
/* Team 1 shooting logic */
void shooting_logic_1_0(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_1_1(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_1_2(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_1_3(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_1_4(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_1_5(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}

/* Team 2 shooting logic */
void shooting_logic_2_0(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_2_1(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_2_2(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_2_3(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_2_4(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
void shooting_logic_2_5(struct Player *self, struct Scene *scene)
{
    (void)scene;
    basic_shoot(self, scene);
    limit_shooting(self, scene);
    limit_kickoff(self, scene);
    limit_out(self, scene);
    prevent_own_goal_shot(self, scene);
}
static bool is_kickoff_state(struct Player *self, struct Scene *scene)
{
    if (self->kit != 0)
        return false;

    if (scene->state != STATE_RESTARTING)
        return false;

    {
        float side_multiplier = (self->team == 1) ? 1.0f : -1.0f;
        float target_x = (float)CENTER_X + (side_multiplier * 15.0f);
        float target_y = (float)CENTER_Y;
        const float eps = 1.0f;

        return (fabsf(self->position.x - target_x) < eps) &&
               (fabsf(self->position.y - target_y) < eps);
    }
}
static void check_moving_possessor(struct Player *self, struct Scene *scene)
{
    struct Team *opp_team = (self->team == 1) ? scene->second_team : scene->first_team;
    float attack_dir = (self->team == 1) ? 1.0f : -1.0f;

    bool must_action = false;
    float goal_x = (self->team == 1) ? (PITCH_X + PITCH_W) : PITCH_X;
    float dist_to_goal = fabsf(goal_x - self->position.x);

    if (dist_to_goal < 220.0f)
    {
        must_action = true;
    }
    else
    {

        for (int i = 0; i < PLAYER_COUNT; i++)
        {
            struct Player *opp = opp_team->players[i];
            if (!opp || opp->kit == 3)
                continue;

            float dx = (opp->position.x - self->position.x) * attack_dir;
            float dy = fabsf(opp->position.y - self->position.y);

            if (dx > -10.0f && dx < 60.0f && dy < 35.0f)
            {
                must_action = true;
                break;
            }
        }
    }

    if (must_action)
        self->state = SHOOTING;
    else
        self->state = MOVING;
}
static bool dis_by_kit(struct Player *self, float x, float y, bool current_intercepting)
{
    const float r_def = 110.0f;
    const float r_def_M = 130.0f;
    const float r_haf = 80.0f;
    const float r_haf_M = 100.0f;
    const float r_at = 70.0f;
    const float r_at_M = 90.0f;

    float cx = self->position.x;
    float cy = self->position.y;
    float dx = x - cx;
    float dy = y - cy;
    float d2 = dx * dx + dy * dy;
    if (self->kit == 0)
    {
        if (d2 <= r_at * r_at)
        {
            return true;
        }
        else if (d2 >= r_at_M * r_at_M)
        {
            return false;
        }
    }
    else if (self->kit == 1 || self->kit == 5)
    {
        if (d2 <= r_haf * r_haf)
        {
            return true;
        }
        else if (d2 >= r_haf_M * r_haf_M)
        {
            return false;
        }
    }
    else if (self->kit == 2 || self->kit == 4)
    {
        if (d2 <= r_def * r_def)
        {
            return true;
        }
        else if (d2 >= r_def_M * r_def_M)
        {
            return false;
        }
    }
    return current_intercepting;
}
static void check_moving_NONpossessor(struct Player *self, struct Scene *scene)
{

    if (self == scene->ball->possessor)
    {
        return;
    }

    if (scene->ball->possessor == NULL)
    {
        struct Vec2 a = {
            self->position.x - scene->ball->position.x,
            self->position.y - scene->ball->position.y};
        struct Vec2 b = {
            scene->ball->velocity.x,
            scene->ball->velocity.y};
        float ball_speed2 = b.x * b.x + b.y * b.y;
        bool ball_moving = ball_speed2 > 25.0f;
        bool toward_self = dotProduct(&a, &b) > 0.0f;

        bool near_enough = dis_by_kit(self, scene->ball->position.x, scene->ball->position.y, self->state == INTERCEPTING);
        if (near_enough && (toward_self || !ball_moving))
        {
            self->state = INTERCEPTING;
        }
        else
        {
            self->state = MOVING;
        }
        return;
    }
    if (self->team == scene->ball->possessor->team)
    {
        self->state = MOVING;
        return;
    }
    else
    {
        if (dis_by_kit(self, scene->ball->possessor->position.x, scene->ball->possessor->position.y, self->state == INTERCEPTING))
        {
            self->state = INTERCEPTING;
        }
        else
        {
            self->state = MOVING;
        }
    }
}
static void goalkeeper_state(struct Player *self, struct Scene *scene)
{
    const float box_w = 80.0f;
    const float box_h = 200.0f;
    const float catch_r = PLAYER_RADIUS + BALL_RADIUS + 14.0f;
    const float catch_d2 = catch_r * catch_r;

    float box_top = CENTER_Y - box_h * 0.5f;
    float box_bottom = CENTER_Y + box_h * 0.5f;

    if (self == scene->ball->possessor)
    {
        self->state = SHOOTING;
        return;
    }
    if (scene->ball->possessor == NULL)
    {
        float bx = scene->ball->position.x;
        float by = scene->ball->position.y;

        bool ball_in_our_half = (self->team == 1) ? (bx <= CENTER_X) : (bx >= CENTER_X);
        bool ball_in_gk_box = (self->team == 1)
                                  ? (bx >= PITCH_X && bx <= PITCH_X + box_w && by >= box_top && by <= box_bottom)
                                  : (bx <= PITCH_X + PITCH_W && bx >= PITCH_X + PITCH_W - box_w && by >= box_top && by <= box_bottom);

        struct Vec2 to_self = {
            self->position.x - bx,
            self->position.y - by};

        struct Vec2 v = {
            scene->ball->velocity.x,
            scene->ball->velocity.y};

        float dis2 = to_self.x * to_self.x + to_self.y * to_self.y;
        float speed2 = v.x * v.x + v.y * v.y;

        bool ball_moving = speed2 > 25.0f;
        bool toward_self = dotProduct(&to_self, &v) > 0.0f;
        bool can_catch = dis2 <= catch_d2;

        if (ball_in_our_half && ball_in_gk_box)
        {

            if (can_catch || (ball_moving && toward_self))
                self->state = INTERCEPTING;
            else
                self->state = MOVING;
        }
        else
        {
            self->state = MOVING;
        }

        return;
    }

    if (scene->ball->possessor->team == self->team)
    {
        self->state = MOVING;
        return;
    }

    {
        float px = scene->ball->possessor->position.x;
        float py = scene->ball->possessor->position.y;

        bool opp_in_our_half = (self->team == 1) ? (px <= CENTER_X) : (px >= CENTER_X);
        bool opp_in_gk_box = (self->team == 1)
                                 ? (px >= PITCH_X && px <= PITCH_X + box_w && py >= box_top && py <= box_bottom)
                                 : (px <= PITCH_X + PITCH_W && px >= PITCH_X + PITCH_W - box_w && py >= box_top && py <= box_bottom);

        float dx = self->position.x - px;
        float dy = self->position.y - py;
        float dis2 = dx * dx + dy * dy;

        if (opp_in_our_half && opp_in_gk_box && dis2 <= catch_d2)
            self->state = INTERCEPTING;
        else
            self->state = MOVING;
    }
}
static void changing_state(struct Player *self, struct Scene *scene)
{
    if (self == scene->ball->possessor)
    {
        if (is_kickoff_state(self, scene))
            self->state = SHOOTING;
        else
            check_moving_possessor(self, scene);
    }
    else
    {
        check_moving_NONpossessor(self, scene);
    }
    if (self->kit == 3)
    {
        goalkeeper_state(self, scene);
    }
}

/* Team 1 change_state logic */
void change_state_logic_1_0(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_1_1(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_1_2(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_1_3(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_1_4(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_1_5(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}

/* Team 2 change_state logic */
void change_state_logic_2_0(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_2_1(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_2_2(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_2_3(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_2_4(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}
void change_state_logic_2_5(struct Player *self, struct Scene *scene)
{
    changing_state(self, scene);
}

/* -------------------------------------------------------------------------
 * Lookup tables for factory
 * ------------------------------------------------------------------------- */
static PlayerLogicFn team1_movement[6] = {
    movement_logic_1_0, movement_logic_1_1, movement_logic_1_2,
    movement_logic_1_3, movement_logic_1_4, movement_logic_1_5};

static PlayerLogicFn team2_movement[6] = {
    movement_logic_2_0, movement_logic_2_1, movement_logic_2_2,
    movement_logic_2_3, movement_logic_2_4, movement_logic_2_5};

static PlayerLogicFn team1_shooting[6] = {
    shooting_logic_1_0, shooting_logic_1_1, shooting_logic_1_2,
    shooting_logic_1_3, shooting_logic_1_4, shooting_logic_1_5};

static PlayerLogicFn team2_shooting[6] = {
    shooting_logic_2_0, shooting_logic_2_1, shooting_logic_2_2,
    shooting_logic_2_3, shooting_logic_2_4, shooting_logic_2_5};

static PlayerLogicFn team1_change_state[6] = {
    change_state_logic_1_0, change_state_logic_1_1, change_state_logic_1_2,
    change_state_logic_1_3, change_state_logic_1_4, change_state_logic_1_5};

static PlayerLogicFn team2_change_state[6] = {
    change_state_logic_2_0, change_state_logic_2_1, change_state_logic_2_2,
    change_state_logic_2_3, change_state_logic_2_4, change_state_logic_2_5};

/* -------------------------------------------------------------------------
 * Factory functions
 * ------------------------------------------------------------------------- */
PlayerLogicFn get_movement_logic(int team, int kit)
{
    if (coach_both_teams)
        return team1_movement[kit];
    return (team == 1) ? team1_movement[kit] : team2_movement[kit];
}

PlayerLogicFn get_shooting_logic(int team, int kit)
{
    if (coach_both_teams)
        return team1_shooting[kit];
    return (team == 1) ? team1_shooting[kit] : team2_shooting[kit];
}

PlayerLogicFn get_change_state_logic(int team, int kit)
{
    if (coach_both_teams)
        return team1_change_state[kit];
    return (team == 1) ? team1_change_state[kit] : team2_change_state[kit];
}

/* -------------------------------------------------------------------------
 * TALENTS
 *  TODO 2: Replace these default values with your desired skill points.
 * ------------------------------------------------------------------------- */
/* Team 1 */
static struct Talents team1_talents[6] = {
    {2, 5, 6, 7},
    {2, 5, 6, 7},
    {7, 5, 3, 5},
    {8, 5, 3, 4},
    {4, 5, 3, 8},
    {3, 6, 7, 4},
};

/* Team 2 */
static struct Talents team2_talents[6] = {
    {2, 5, 5, 8},
    {2, 3, 8, 7},
    {7, 4, 4, 5},
    {8, 6, 1, 5},
    {5, 2, 5, 8},
    {3, 8, 5, 4},
};

struct Talents get_talents(int team, int kit)
{
    if (coach_both_teams)
        return team1_talents[kit];
    return (team == 1) ? team1_talents[kit] : team2_talents[kit];
}

/* -------------------------------------------------------------------------
 * Positioning
 *  TODO 3: Decide players positions at kick-off.
 *        Players must stay on their half, outside the center circle.
 *        Keep in mind that the kick-off team's first player will automatically
 *             be placed at the center of the pitch.
 * ------------------------------------------------------------------------- */
/* Team 1 */
static struct Vec2 team1_positions[6] = {
    {330, CENTER_Y},
    {270, CENTER_Y - 100},
    {170, CENTER_Y - 95},
    {60, CENTER_Y},
    {170, CENTER_Y + 95},
    {270, CENTER_Y + 100},
};

/* Team 2 */
static struct Vec2 team2_positions[6] = {
    {750, CENTER_Y},
    {760, CENTER_Y - 100},
    {830, CENTER_Y - 75},
    {940, CENTER_Y},
    {830, CENTER_Y + 75},
    {760, CENTER_Y + 100},
};

struct Vec2 get_positions(int team, int kit)
{
    return (team == 1) ? team1_positions[kit] : team2_positions[kit];
}
