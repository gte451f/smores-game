// A bandit shakedown - the conversation-player test scene, written in Ink.
// The same scene is in shakedown.yarn, so the two languages can be read side by side.
//
// gold() is a question the game answers. TakeMoney and ChangeStanding are actions the game
// carries out. asked_about_road is Ink's own memory, local to this conversation.
// Ink has no line ids of its own, so each line carries an #id: tag by hand - that id is what
// crosses the network, and what a translation keys on.

EXTERNAL gold()
EXTERNAL TakeMoney(amount)
EXTERNAL ChangeStanding(faction, amount)

VAR asked_about_road = false

Bandit: Toll road. Twenty gold, or you walk back the way you came. #id:shakedown_toll
- (choices)
+ {gold() >= 20} [Pay the toll. #id:shakedown_pay]
    ~ TakeMoney(20)
    Bandit: Pleasure doing business. Road's yours. #id:shakedown_paid
    -> END
+ {not asked_about_road} [Who says it's your road? #id:shakedown_ask]
    ~ asked_about_road = true
    Bandit: The twelve of us in those rocks say so. #id:shakedown_twelve
    -> choices
+ [Get out of my way. #id:shakedown_refuse]
    ~ ChangeStanding("bandits", -10)
    Bandit: Wrong answer. #id:shakedown_wrong
    -> END
