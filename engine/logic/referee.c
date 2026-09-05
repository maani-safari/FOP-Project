#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "referee.h"
#include "game/possession.h"
#include "entities/team.h"

/**
 * @brief Determines whether a goal has been scored.
 *
 * This function checks if the ball has completely crossed either goal line
 * while also being fully inside the vertical goal mouth.
 *
 * Important details:
 * - A goal is only valid if the *entire ball* (taking BALL_RADIUS into account)
 *   has crossed the goal line.
 * - The ball must be vertically contained between the goal posts.
 * - The right goal corresponds to Team 1 scoring.
 * - The left goal corresponds to Team 2 scoring.
 * @return
 * - 1 if Team 1 scores,
 * - 2 if Team 2 scores,
 * - 0 if no goal has occurred.
 */
static int goal(float x, float y)
{
    // TODO 1: implement this function
    // You must check for and print these EXACT logs:
    if ((x < PITCH_X - BALL_RADIUS) && (y < CENTER_Y + (GOAL_HEIGHT / 2) - BALL_RADIUS) && (y > CENTER_Y - (GOAL_HEIGHT / 2) + BALL_RADIUS))
    {
        printf("GOAL! Left net hit at x:%.2f, y=%.2f\n", x, y);
        return 2;
    }
    else if ((x > PITCH_X + PITCH_W + BALL_RADIUS) && (y < CENTER_Y + (GOAL_HEIGHT / 2) - BALL_RADIUS) && (y > CENTER_Y - (GOAL_HEIGHT / 2) + BALL_RADIUS))
    {
        printf("GOAL! Right net hit at x:%.2f, y=%.2f\n", x, y);
        return 1;
    }
    else
    {
        return 0; // for now
    }
}

/**
 * @brief Checks whether the ball is out of bounds.
 *
 * The ball is considered out only when its *entire area* lies outside
 * the pitch boundaries. Partial overlap with the pitch does NOT count as out.
 *
 * Notes for students:
 * - Use BALL_RADIUS to ensure the whole ball has crossed a boundary.
 * - All four pitch sides (left, right, top, bottom) must be considered.
 * - This function does not handle goals; goal detection is performed separately.
 * @return true if the ball is fully out of bounds, false otherwise.
 */
static bool out(float x, float y)
{
    // TODO 2: implement this function
    // You must check for and print this EXACT log:
    if (y < PITCH_Y - BALL_RADIUS || y > PITCH_Y + PITCH_H + BALL_RADIUS)
    {
        printf("Ball is out: x=%.2f, y=%.2f\n", x, y);
        return true;
    }
    if ((x < PITCH_X - BALL_RADIUS || x > PITCH_X + PITCH_W + BALL_RADIUS) &&
        !(y > CENTER_Y - GOAL_HEIGHT / 2 + BALL_RADIUS &&
          y < CENTER_Y + GOAL_HEIGHT / 2 - BALL_RADIUS))
    {
        printf("Ball is out: x=%.2f, y=%.2f\n", x, y);
        return true;
    }

    return false;
}

/**
 * @brief Acts as the game referee for one simulation step.
 *
 * This function is responsible for detecting and resolving game events
 * related to the ball, such as goals and out-of-bounds situations.
 *
 * Responsibilities:
 * - Check for goals BEFORE checking for out-of-bounds.
 * - Update team scores if a goal is detected.
 * - Report the appropriate game event.
 *
 * Notes for students:
 * - A goal must be checked first because a scored goal is technically out.
 * - If no event occurs, the game continues normally.
 *
 * @param scene Pointer to the current game scene.
 *
 * @return
 * - GOAL if a goal has been scored,
 * - OUT if the ball is out of bounds,
 * - 0 if no event occurred.
 */
int referee(struct Scene *scene)
{

    // TODO 3: implement this function
    int res = goal(scene->ball->position.x, scene->ball->position.y);
    if (res == 1)
    {
        scene->first_team->score++;
        return GOAL;
    }
    else if (res == 2)
    {
        scene->second_team->score++;
        return GOAL;
    }
    bool ou = out(scene->ball->position.x, scene->ball->position.y);
    if (ou == true)
    {
        return OUT;
    }
    return PLAY_ON; // for now
}

/**
 * @brief Verifies the validity of a player's talent distribution.
 *
 * This function checks whether each individual talent value is within
 * the allowed range and whether the total talent points do not exceed
 * the maximum allowed per player.
 *
 * Notes for students:
 * - Each skill must be between 1 and MAX_TALENT_PER_SKILL (inclusive).
 * - The sum of all skills must not exceed MAX_TALENT_PER_PLAYER.
 * - Invalid configurations should be reported as errors.
 *
 * @param talents The talent structure to validate.
 */
void verify_talents(struct Talents talents)
{
    // TODO 4: implement this function
    // You must check for and print this EXACT error:
    // printf("ERROR: Invalid talents! Values: defence=%d, agility=%d, dribbling=%d, shooting=%d, sum=%d\n",
    // talents.defence, talents.agility, talents.dribbling, talents.shooting, sum);
    int sum;
    sum = talents.agility + talents.defence + talents.dribbling + talents.shooting;
    if ((talents.agility < 1 || talents.agility > MAX_TALENT_PER_SKILL) || (talents.defence < 1 || talents.defence > MAX_TALENT_PER_SKILL) || (talents.dribbling < 1 || talents.dribbling > MAX_TALENT_PER_SKILL) || (talents.shooting < 1 || talents.shooting > MAX_TALENT_PER_SKILL) || (sum > MAX_TALENT_PER_PLAYER))
    {
        printf("ERROR: Invalid talents! Values: defence=%d, agility=%d, dribbling=%d, shooting=%d, sum=%d\n",
               talents.defence, talents.agility, talents.dribbling, talents.shooting, sum);
    }
}

/**
 * @brief Verifies the correctness of a player's current state.
 *
 * Ensures that the player's state is consistent with the game rules.
 * In particular, only the player who currently possesses the ball
 * is allowed to be in the SHOOTING state.
 *
 * Notes for students:
 * - If a player attempts to shoot without possessing the ball,
 *   their state must be corrected.
 *
 * @param player Pointer to the player being verified.
 * @param scene  Pointer to the current game scene.
 */
void verify_state(struct Player *player, struct Scene *scene)
{

    // TODO 5: implement this function
    // You must check for and print this EXACT error:
    // printf(" ERROR: the ball is not yours, you can't shoot! (team %d, player %d)\n",
    //        player->team, player->kit);
    if (scene->ball->possessor != player)
    {
        if (player->state == SHOOTING)
        {
            player->state = IDLE;
            printf(" ERROR: the ball is not yours, you can't shoot! (team %d, player %d)\n",
                   player->team, player->kit);
        }
    }
}

/**
 * @brief Verifies and limits a player's movement speed.
 *
 * This function ensures that a player's velocity does not exceed
 * the maximum allowed speed derived from their agility talent.
 *
 * Notes for students:
 * - Maximum speed scales linearly with the agility talent.
 * - Both x and y velocity components must be checked independently.
 * - If a component exceeds the limit, it must be clamped.
 *
 * @param player Pointer to the player whose movement is being verified.
 */
void verify_movement(struct Player *player)
{

    // TODO 6: implement this function
    // You must check for and print these EXACT errors:
    // printf(" ERROR: Demanding to run too fast in dimension x! (team %d, player %d)\n", player->team, player->kit);
    // printf(" ERROR: Demanding to run too fast in dimension y! (team %d, player %d)\n", player->team, player->kit);
    float limit = MAX_PLAYER_VELOCITY * ((float)player->talents.agility / MAX_TALENT_PER_SKILL);
    if (fabs(player->velocity.x) > limit)
    {
        printf(" ERROR: Demanding to run too fast in dimension x! (team %d, player %d)\n", player->team, player->kit);
        if (player->velocity.x < -(limit))
        {
            player->velocity.x = -(limit);
        }
        else
        {
            player->velocity.x = limit;
        }
    }
    if (fabs(player->velocity.y) > limit)
    {
        printf(" ERROR: Demanding to run too fast in dimension y! (team %d, player %d)\n", player->team, player->kit);
        if (player->velocity.y < -(limit))
        {
            player->velocity.y = -(limit);
        }
        else
        {
            player->velocity.y = limit;
        }
    }
}

/**
 * @brief Verifies the validity of a ball shot.
 *
 * This function ensures that the ball's velocity after a shot does not
 * exceed the maximum allowed speed derived from the shooter's talent.
 *
 * Additional kickoff rules:
 * - During kickoff, the ball must be played into the player's own half.
 *
 * Notes for students:
 * - Ball velocity must be clamped if it exceeds the allowed maximum.
 * - Both velocity components must be checked independently.
 *
 * @param ball    Pointer to the ball being shot.
 * @param kickoff True if the shot occurs during kickoff.
 */
void verify_shoot(struct Ball *ball, bool kickoff)
{

    // TODO 7: implement this function
    // You must check for and print these EXACT errors:
    // printf(" ERROR: Demanding to shoot too fast in dimension x! (team %d, player %d)\n", player->team, player->kit);
    // printf(" ERROR: Demanding to shoot too fast in dimension y! (team %d, player %d)\n", player->team, player->kit);
    // printf(" ERROR: You must pass to your own half! (team %d, player %d)\n", player->team, player->kit);
    float limit = MAX_BALL_VELOCITY * ((float)ball->possessor->talents.shooting / MAX_TALENT_PER_SKILL);
    if (kickoff && ball->position.x==CENTER_X && ball->position.y == CENTER_Y)
    {
        if (ball->possessor->team == 1)
        {
            if (ball->velocity.x > 0.f)
            {
                printf(" ERROR: You must pass to your own half! (team %d, player %d)\n", ball->possessor->team, ball->possessor->kit);
            }
        }
        if (ball->possessor->team == 2)
        {
            if (ball->velocity.x < 0.f)
            {
                printf(" ERROR: You must pass to your own half! (team %d, player %d)\n", ball->possessor->team, ball->possessor->kit);
            }
        }
    }

    if (fabs(ball->velocity.x) > limit)
    {
        printf(" ERROR: Demanding to shoot too fast in dimension x! (team %d, player %d)\n", ball->possessor->team, ball->possessor->kit);
        if (ball->velocity.x < -(limit))
        {
            ball->velocity.x = -(limit);
        }
        else
        {
            ball->velocity.x = limit;
        }
    }
    if (fabs(ball->velocity.y) > limit)
    {
        printf(" ERROR: Demanding to shoot too fast in dimension y! (team %d, player %d)\n", ball->possessor->team, ball->possessor->kit);
        if (ball->velocity.y < -(limit))
        {
            ball->velocity.y = -(limit);
        }
        else
        {
            ball->velocity.y = limit;
        }
    }
}

