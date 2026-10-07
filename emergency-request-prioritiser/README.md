# Emergency Request Prioritiser

A command-line program in C that ranks emergency help requests from a flood or landslide, so the most urgent one gets answered first. Every score comes with a plain reason that anyone can check.

## Why

During the monsoon, Himachal Pradesh faces landslides and flash floods, and requests for help can arrive faster than a control room can answer them. Deciding **who to help first** is the hardest step. I built this while my team was exploring disaster-response ideas for **Hack on Hills 8.0**.

The rules are deliberately simple: a person should be able to look at any ranking and see exactly why it happened.

## How the score works

| Rule | Points |
|---|---|
| Someone is injured | +5 |
| Need is **rescue** or **medical** | +4 |
| Need is **food** or **shelter** | +2 |
| Every 5 people in the group | +1 |

Highest score is answered first. If two requests have the same score, the one that arrived first stays ahead.

The point values are `#define`s at the top of `prioritiser.c`, so they are easy to change.

## Features

- Reads requests from a text file (`requests.txt` by default)
- Ranks them by urgency and shows **why** each one got its score
- Summary by village: requests, people and injuries per area
- Add new requests from the keyboard (they go through the same checks as the file)
- Saves the list back to the file
- **Bad lines are never silently ignored.** Each one is reported with its line number and the reason, and the program warns you before saving if any were skipped

## How to run

```
gcc prioritiser.c -o prioritiser
prioritiser            (Windows)
./prioritiser          (Linux / macOS)
```

To use a different data file: `prioritiser myfile.txt`

## Input format

One request per line. Lines starting with `#` are comments.

```
name,village,people,need,injured
Ramesh,Kullu,6,rescue,yes
```

- `need` is one of: `rescue`, `medical`, `food`, `shelter`
- `injured` is `yes` or `no`

## Sample run

```
=== Emergency Request Prioritiser ===
Skipped line 14: need must be rescue, medical, food or shelter (got "water")
Loaded 8 requests from requests.txt (1 line skipped - see above)

PRIORITY QUEUE - answer from the top
Rank Score Name       Village    People  Need     Why this score
----------------------------------------------------------------------------
#1   10    Ramesh     Kullu           6  rescue   injured +5, rescue +4, 6 people +1
#2   9     Pooja      Shimla          3  medical  injured +5, medical +4, 3 people +0
#3   8     Anita      Kullu           5  shelter  injured +5, shelter +2, 5 people +1
#4   5     Vikram     Kinnaur         9  rescue   rescue +4, 9 people +1
#5   5     Rahul      Mandi          15  food     food +2, 15 people +3
#6   4     Sunita     Mandi           4  medical  medical +4, 4 people +0
#7   4     Arjun      Solan          12  food     food +2, 12 people +2
#8   2     Meena      Chamba          2  shelter  shelter +2, 2 people +0

SUMMARY BY VILLAGE
Village      Requests   People    With injuries
-----------------------------------------------
Kullu               2       11                2
Mandi               2       19                0
Solan               1       12                0
Chamba              1        2                0
Kinnaur             1        9                0
Shimla              1        3                1
-----------------------------------------------
Total               8       56                3
```

The `water` line in the sample file is wrong on purpose, to show how bad data is reported.

## C concepts used

Structs, arrays of structs, file reading and writing (`fopen`, `fgets`, `fprintf`), string handling (`strtok`, `strtol`, `snprintf`), input validation, bubble sort, and a menu loop.

## Limitations

- **Prototype with sample data.** It is not used in real emergencies, and the names and requests in `requests.txt` are made up.
- The rules are a simple starting point. Real triage is done by trained responders.
- Village names must match exactly to be grouped together (`Mandi` and `mandi` count as two villages).
- Holds up to 100 requests.

## Ideas for next steps

- Mark a request as answered and remove it from the queue
- Record the time each request arrived
- Group villages without caring about capital letters
