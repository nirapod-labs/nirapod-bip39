# AI Writing Detection Patterns Database

Complete database of patterns that AI detection tools flag. Organized by detection signal strength.

## Table of contents

1. [Structural patterns](#structural-patterns)
2. [Phrase patterns](#phrase-patterns)
3. [Statistical signals](#statistical-signals)
4. [Detection tool thresholds](#detection-tool-thresholds)

---

## Structural patterns

### Very high detection signal

**Em dash overuse:** AI uses em dashes 2–5x more than human baseline (2.5–2.75 per 1000 words in human writing). GPT-4o uses roughly 10x more than GPT-3.5. Used formulaically: "it's not X - it's Y." Keep to max 1 per 500 words.

**Uniform sentence length:** AI defaults to roughly 15-word sentences with low standard deviation. Human writing has high variance - mixing 3-word punches with 40-word complex sentences.

**Low burstiness:** Consistent rhythm with no variation between short punchy sentences and long complex ones. Human burstiness (structured text): mean 83–95 (SD 17–38). AI burstiness: mean 64–70 (SD 8–31). For unstructured text the gap is massive - human mean 271 vs AI mean 25.

**Parallel negation:** "It's not just X, it's Y" / "Not only X, but also Y" / "X isn't just about Y - it's about Z." The single most prominent AI tell per Wikipedia editors.

**Bolded bullet headers:** "**Scalability**: The system is designed to..." - virtually nonexistent in human writing but AI does this constantly.

### High detection signal

**Rule of three:** Overuse of triplet structures: "adjective, adjective, and adjective" or "X, Y, and Z" to seem comprehensive.

**Synonym cycling:** Repetition-penalty causes AI to use different synonyms for the same term across paragraphs ("protagonist" then "key player" then "eponymous character"). Humans just repeat the word.

**Colon stacking:** Colons mid-paragraph plus colons before bullet lists in the same section.

**Formulaic conclusions:** Starting with "Overall," "In conclusion," "In summary" then repeating prior content.

**Challenges then Future Prospects:** AI articles include a "Challenges" section followed by "Future Prospects" with vaguely positive assessment.

**Present participial clause overuse:** AI uses these at 2–5x human rate. "Bryan, leaning on his agility, dances around the ring."

**Uniform paragraph length:** Multiple consecutive paragraphs of similar size.

**Importance inflation:** Connecting minor details to "broader themes" of significance even when unwarranted.

**Perfect grammar:** Zero errors, zero colloquialisms, zero informal constructions.

### Medium detection signal

**Nominalization overuse:** Abstract noun forms instead of verbs: "development" instead of "develop." AI uses 1.5–2x human rate.

**Curly quotation marks:** ChatGPT uses curly quotes ("…") and apostrophes (') instead of straight ones. A formatting tell.

**Title Case + colon headers:** "Topic: The Subtitle" format. Humans use sentence case.

**Weasel wording:** "Experts suggest," "studies show," "it has been widely recognized" without citation.

**-ing endings (empty analysis):** Ending sentences with significance commentary: "…improving convenience," "…highlighting its significance."

---

## Phrase patterns

### Templated intros and openers (never use)

- "In today's fast-paced [world/landscape/business environment]…" (107x more frequent in AI)
- "In today's digital age"
- "In a world where…"
- "As the [industry/landscape] continues to evolve…"
- "Now more than ever…"
- "As [broad trend] continues to [vague verb], [audience] must [generic goal]"
- "In today's competitive business environment…"
- "As businesses navigate the evolving landscape of…"
- "With the rise of [technology/trend]…"
- "Imagine a world where…"

### Business hype (never use)

- "Revolutionize the way"
- "Unlock the power/potential of"
- "Unleash the power of"
- "A game-changer for [industry/role]"
- "That's where [product/solution] comes in"
- "Supercharge your [workflow/team/performance]"
- "Transforming [X] into [Y]"
- "Stay ahead of the curve"
- "Future-proof your…"
- "Forward-thinking companies…"
- "Strategic advantage/benefit"
- "Thrilled to announce…"
- "Simply cannot [match/achieve/replicate]"
- "Best-in-class"
- "Next-generation"

### Clichéd significance claims (never use)

- "Play a significant/crucial role in shaping" (182x more frequent in AI)
- "A pivotal moment in…"
- "A broader movement"
- "Marking a significant shift toward…"
- "Emphasizing the significance of…"
- "Reflecting the continued relevance of…"
- "A testament to"
- "Significant milestone"
- "It cannot be overstated"
- "Serves as a [beacon/reminder/testament]"
- "Enduring legacy"

### Filler authority phrases (never use)

- "It's important to note that"
- "It's worth mentioning that"
- "It is crucial to understand"
- "It is essential to consider"
- "In light of the fact that"
- "Given the fact that"
- "Bearing in mind that"
- "As a matter of fact"
- "It is generally considered"
- "Research needed to understand"
- "Objective study aimed" (269x more frequent in AI)

### Faux-conversational (never use)

- "Let's dive in / Let's delve in"
- "Let's break it down"
- "Let's uncover"
- "Let's face it…"
- "But let's get real…"
- "Here's the thing…"
- "Here's the uncomfortable truth:"
- "The goal?" / "The result?" / "The bottom line"
- "The good news?"
- "The real unlock"
- "What does this mean for you?"
- "Not all [X] are created equal"
- "It's no secret that…"
- "Whether you're a [persona A] or a [persona B]…"
- "I hope this message finds you well"

### Negation/contrast structures (very high AI signal)

The single most prominent AI tell per Wikipedia editors:

- "It's not just X, it's Y"
- "Not only X, but also Y"
- "It's not X - it's Y"
- "X isn't just about Y - it's about Z"
- "X doesn't [verb]; it [verb]s"
- "This isn't a retreat from X; it's an evolution"
- "X is more than just Y. It's Z."

### Template structures (never use)

- "[Problem]? Meet [solution]."
- "It's time to…"
- "Why it matters…"
- "Here's what you need to know…"
- "Do X, so you can Y."
- "Master the art of"
- "In conclusion"
- "In summary"
- "Overall," (as paragraph opener)
- "Without further ado"
- "On the other hand" (as mechanical transition)
- "Having said that"
- "It goes without saying"
- "When it comes to" (as hedge filler)
- "In terms of" (as hedge filler)
- "The fact of the matter is"

### Metaphorical clichés (never use)

- "Delve into the world of"
- "Pave the way for"
- "At the forefront of"
- "Harness the power of"
- "Embark on a journey"
- "Push the boundaries of"
- "A gateway to"
- "Bridging the gap between"
- "Capitalize on the opportunities"
- "Navigate the complexities"
- "Lay the groundwork for"
- "Foster a culture of"
- "In the realm of"
- "Rich tapestry"

### Wikipedia-specific AI phrases (avoid in encyclopedic writing)

- "Notable works include" (120x more frequent in AI)
- "Aims to explore/enhance" (50x more frequent)
- "Despite facing"
- "Expressed excitement"
- "Today's fast-paced world" (107x more frequent)
- "Showcasing" (20x more frequent)
- "Aligns" (16x more frequent)

### Chatbot artifacts (never use)

- "Great question!"
- "That's a really interesting point"
- "Absolutely! Let me…"
- "Certainly! Here's…"
- "I hope this helps!"
- "As an AI…"
- "As a language model…"

### Formal transitions (replace with informal alternatives)

Never use: Moreover, Furthermore, Additionally, Indeed, Subsequently, Accordingly, Consequently, Thus, Therefore, Notably

Instead use: Anyway, So yeah, The thing is, Point being, That said, Look, OK so, Moving on, Speaking of which, On another note, Honestly though, Real talk

---

## Statistical signals

| Metric | Human writing | AI writing |
|--------|-------------|-----------|
| Perplexity (structured) | Mean 105–111 (SD 30–36) | Mean 59–61 (SD 10–20) |
| Perplexity (unstructured) | Mean 165.57 (SD 55.89) | Mean 47.70 (SD 18.15) |
| Perplexity detection threshold | >85 likely human | <20 likely AI |
| Burstiness (structured) | Mean 83–95 (SD 17–38) | Mean 64–70 (SD 8–31) |
| Burstiness (unstructured) | Mean 271.13 (SD 459.07) | Mean 24.90 (SD 15.49) |
| Em dashes per 1000 words | 2.5–2.75 | 5–14 (2–5x human) |
| Passive voice usage | Normal baseline | ~50% of human rate (GPT-4o underuses passive) |
| Present participial clauses | Normal baseline | 2–5x human rate |
| Nominalizations | Normal baseline | 1.5–2x human rate |
| Voice variation | Baseline | 23% less variation than human |
| Sentence length variance | High (Fano factor high) | Low (Fano factor low) |
| Contraction rate | High in casual writing | Low - defaults to formal |
| Active/passive ratio target | ~65% active / 35% passive | ~95% active / 5% passive |

---

## Detection tool thresholds

| Tool | AI text accuracy | False positive rate |
|------|-----------------|---------------------|
| Turnitin | ~100% | ~8% |
| GPTZero | 97–100% (claimed) | 1–2% (claimed), ~15% (independent) |
| Originality.ai | 97–100% | Variable |
| ZeroGPT | 70–80% (real-world) | Higher |

**Note:** AI detection tools evolve constantly. Companies read detection lists and adjust model outputs. "Delve" was massively overused in 2023–early 2024, then dropped sharply. Em dash overuse emerged with GPT-4o and persists across all major models. OpenAI's GPT-5.1 attempted to suppress em dash usage. The Carnegie Mellon PNAS 2025 study found instruction tuning (RLHF) is the primary driver of AI writing style discrepancies, creating an "informationally dense, noun-heavy style." Patterns need periodic review.
