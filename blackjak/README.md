# Blackjack

A new C implementation of the house rules in the accompanying
`blackjac.bas`, a historical BASIC listing attributed to an unknown author.
`rules.c` scores hands, checks actions, settles wagers and shuffles the deck
using a supplied random picker. `blackjac.c` handles the console and the
round sequence. The game uses standard C console calls and CP437 box drawing characters
for the cards. Suits use printable letters (S, D, H, C). The dealer's hole card
shows a patterned back until the reveal. The CP/M-86 build uses the VT52 color
extension for bright red hearts and diamonds and bright blue clubs and spades.
It returns to the normal foreground after each colored card row. The DOS
build uses the standard console color.

Use `BLACKJAK -g` on a PC text console that draws the CP437 glyphs for
bytes 03h–06h (hearts, diamonds, clubs, spades). Use the default suit letters
on emu2's host-terminal output or a serial terminal.

Build with `make` using Aztec C 4.3. Run `BLACKJAK` on CP/M-86
(`blackjak.cmd`) or DOS (`blackjak.com`). Enter a whole-dollar wager from
1 to 500, 7777 to reshuffle, or 0 to quit. During play enter 0 to stand,
1 to hit, 2 to double, or 3 to split. Accept insurance with 1 or decline
with 0 when the dealer shows an ace.

- Blackjack pays 3:2. Simultaneous naturals push. A 21 after splitting
  counts as an ordinary 21.
- The dealer hits soft 17, following the BASIC game logic.
- Split equal ranks once. Split aces receive one card each. Doubling
  is allowed on the first decision of a playable hand and draws one card.
- The deck is shuffled between rounds when too few cards remain for a
  full round. The running balance tracks net wins and losses in half dollars.

`make test` checks the rules with the host C compiler, then smoke-tests
the production binaries under emu2 (CP/M profiles 2.2, 3.1 and 4.1, plus DOS).
