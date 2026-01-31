# SpectralAudit

I present, SpectralAudit. 

SpectralAudit is a C++ project built to explore musical trends over time. Specifically, I wanted to study primarily loudness, how genres evolve, how loudness and spectral balance shift by year, and where the outliers live.
On my hard drive, I maintain an ever-growing, meticulously hand-crafted library of ~25,000 MP3 files in 320 kbps, organized by genre, artist, album, and year.
To analyze it at scale, I built a high-performance, multi-threaded audio pipeline in C++ that processed the entire library in under an hour; decoding MP3s, running STFT-based spectral analysis, and persisting the results into SQLite for querying. 

_If you’re not interested in the technical details, you can skip Section 1 and jump straight to Section 2 for the dataset and scope, or Section 3 for the results._

## 1. Technical

### A. Overview
The program is written in C++. It uses minimp3 for decoding audio, FFTW for STFT and SQLite for the persistence layer. 

A single producer thread walks the filesystem and feeds MP3 paths into a bounded queue. Worker threads pull from that queue, perform decoding and STFT-based analysis, and push completed results into a sink that streams them into SQLite.
The bounded queue acts as backpressure between disk I/O and CPU-heavy DSP, keeping the pipeline saturated without letting memory run away.

### B. Performance analysis 

All benchmarks were run on the following system:
- CPU: Intel i7-13700K (16 cores)
- RAM: 32 GB
- Storage: 7200 RPM HDD
- Window size: 2048
- Hop size: 512
- Files: .mp3 (320 kbps exclusively)

Final results for processing and persisting ~25,000 tracks:

<code>Processed: 24978, Failed: 19, Enqueued: 24997
Total time: 00:50:13.417</code>

CPU time was dominated (as expected) by 
- STFT transform (~40%),
- .mp3 Decoding (~25%)
- Feature extraction (~20%).

The remaining ~15% consisted of miscellaneous functionality, with no single component exceeding ~2%.

A total of 14 threads were used:
- 1 producer thread iterating the filesystem
- 12 worker threads performing audio decoding and analysis
- 1 SQLite consumer thread streaming batched inserts into the database

CPU usage stayed consistently near 70%, indicating effective utilization without hard saturation. No CPU bottlenecks were observed.

Total system memory fluctuated between 11 GB and 20 GB, depending on the duration of tracks being processed. No memory bottlenecks or instability were observed.

HDD usage fluctuated as expected based on worker completion timing. Because audio processing is expensive, the producer thread was able to keep the queue populated without overwhelming disk I/O, preventing the HDD from becoming a limiting factor.

Below, you'll find the specific stages development went through. 
Each commit represents a specific state change of optimization. The benchmarks were run on a test folder consisting of 344 Doom Metal tracks. This section will focus on performance improvements over the audio processing itself, highlighting key scaling features required. 

- V0.1 - Baseline

The baseline was the first implementation of the processing. No attempt was made to optimize anything save for a few minor things. Single threaded analysis and individual SQLite inserts per track.

<code>Analysis time: 00:06:15.921
DB write time: 00:00:02.791
Total time:   00:06:18.712
Tracks analyzed: 344
</code>

The results speak for themselves. Feature analysis and STFT computation was dominant. 


- V0.2 CPU improvements

This version improves on feature extraction by reducing the number of passes rover the data. The net gain was a modest 4.3% performance improvement.
While not particularly impressive in isolation, when extrapolated to the full 25.000-track library, it translates to roughly 20 minutes shaved off the total runtime making it a more-than-worthwhile win. 

<code>Processed: 344, Failed: 0
Analysis time: 00:05:59.434
DB write time: 00:00:02.860
Total time:   00:06:02.295
Tracks analyzed: 344</code>

- V0.3 Parallelism and batch insert

This version introduced the largest performance gains via a producer/consumer threading model, parallel audio processing and batched SQLite inserts. Processing became over _**~7 times faster**_, reducing total runtime from ~6 minutes to ~49 seconds for the benchmark dataset.

<code>Processed: 344, Failed: 0, Enqueued: 344
Analysis time: 00:00:49.793
DB write time: 00:00:00.010
Total time:   00:00:49.804
Tracks analyzed: 344</code>

- V0.4 Structure improvements.

This version focused on code structure and maintainability. Clearer separation of responsibilities and more defensive handling of edge cases. No performance changes. 

- V1.0 SqliteStreaming - Final Version

The final version removes the last meaningful scaling constraint. Results are no longer accumulated in memory; instead, a dedicated SQLite worker thread streams batched inserts as tracks are processed. Multithreaded FFTW handling was also tightened up.
At this point, the pipeline scales cleanly to the full library size and represents the current stable state of the project.

There is one remaining consideration: frame-level data for a track is currently held in memory until aggregation completes. In pathological cases -- very long tracks such as 1h+ DJ sets or large concatenated playlists -- peak memory usage could spike if multiple workers hit such files simultaneously.
This would be straightforward to address by switching to incremental aggregation, but given that this is my private library and I know that such tracks are rare, there's no point in doing it. Even in the worst realistic case, available system memory is sufficient to absorb the spike without becoming a bottleneck.

### C. Black Metal

Black metal artists love using weird characters in titles which caused various problems with audio decoding libraries and the default windows console. 
To avoid path and Unicode issues during decoding, BlackMetalSanitizer temporarily copies each track to a filesystem-safe, ASCII-only path, which is then passed to the decoder. The temporary file is cleaned up immediately afterward, and the original library is never modified.

## 2. Data

This section addresses common and reasonable questions about the dataset and the conclusions drawn from it. While the project was built with care toward validity, accuracy, and internal correctness, it is ultimately a personal effort developed in my free time out of a love for software engineering and music.

The results are presented in a structured and professional manner, but they are not peer-reviewed and should not be interpreted as formal research. The dataset reflects my own private music library, along with the biases, gaps, and quirks that naturally come with it.

This is what the library looks like in Wiz64. 
<img width="1043" height="671" alt="image" src="https://github.com/user-attachments/assets/d803e699-2c62-4835-8974-2f21cea9114e" />


1. The original library contains ~27.000 files. Of those, 25.000 were processed because some files are album cover images, .flac or not 320kbps.
2. Every single .mp3 file that was processed was 320kbps.
4. I do not collect albums of live performances. Almost all inserted songs are studio performances exclusively. Bonus tracks of live performances in albums were filtered out in post-processing. 
5. DJ Sessions, Mixtapes, or albums/sessions that exist in one, large continuous .mp3 do not exist in this library thereby, none were processed. 
6. Any song for which an album was not released because it came out as a single is only included in queries regarding Genre or Artist data according to its classification.
7. No loudness normalization or post-processing was applied; all tracks were analyzed in their original encoded form.
9. Stereo audio was downmixed to mono prior to analysis to simplify spectral aggregation and ensure consistency across tracks.
10. There's a healthy mix of mainstream and underground music. I don't favor one or the other.
11. Industrial - Electronic - DnB - Hybrids folder contains subfolders for electronic music. Those were post-processed into their respective genres accordingly.
12. Unknown - Playlists - Uncategorized were post-processed in the DB to correctly be categorized in the genre they belong.
13. Tags (Artist, Track Title, Album, Year) do not come from a single authoritative source. They were manually processed during insertion to the library. Like I mentioned, this was handcrafted over several years.
14. Genre was inserted in post according (largely) to the folder structure seen above.
15. The music I create/produce is not included in this library.
16. Origin countries are not included anywhere. I am not even sure if I have that information to even extract stats for it. 
17. Regarding genres and peculiarities of music.
      - Not all genres/subgenres are included. Obviously, I listen to the things I like.  
      - Grindcore and Deathgrind are notoriously short. I usually do not collect those. While still shorter on average than other genres, they still retain an acceptable level of length. However, novelty songs were not processed. (No 10second grindcore meme songs)
      - Heavy, Hair and Power Metal folder does not contain any power metal. I dislike it. The name stuck during folder creation.
      - No hyper-fragmented genres with three monthly listeners on Spotify exist. Atmospheric Post-Occult Raw Frostbitten Melancholic Nordic Depression Doom Metal goes into Doom Metal.
      - Similarly for electronic music, Dark Forest Ritual Jungle Drum & Bass goes straight into Drum and Bass. 
      - Guitarists & Solo Work contains albums released by "solo" musicians. All of them are some type of metal. They too have been post-processed into the database but in stats, will appear as solo work. In hindsight, I should have called it something else but it is what it is.
   
19. Intros, preludes, interludes, spoken-word, ambient and noise tracks are included if they were part of an album release. During querying, filtering was used to exclude them if they were unreasonable outliers.


## 3. Results

### 1. Spectral Loudness

The first thing I wanted to look at was which genre is loudest on average. This chart shows that by using the median PCM RMS per track, with silence filtered out. In simple terms, it shows which genres tend to be recorded the loudest overall, 
after ignoring quiet parts like intros, outros, or silence. Genres higher on the chart usually sound louder because their music stays loud most of the time, not because you’ve turned the volume knob up.
<img width="1366" height="890" alt="1  Typical Loudness by Genre" src="https://github.com/user-attachments/assets/0fa1da1f-aff5-492b-a1d3-1d2924b03f8a" />

What immediately stands out is that the loudest genre by a wide margin is Phonk; a genre that became popular largely through TikTok and Instagram car culture.

What surprised me wasn’t just that Phonk was at the top, but how far it was from everything else. The gap between Phonk and the next loudest genres is enormous. 
In fact, for any audio engineers reading this, Phonk’s RMS values are so high that your first reaction is probably that something is wrong with the data. It was mine at least. When I saw the numbers, my immediate assumption was that I had made a mistake somewhere in the processing. Keep this point in mind, we're gonna come back to it before we move away from loudness-related data. 

And so I moved on to checking 95th percentile loudness. This chart looks at peak loudness behavior, using the 95th percentile PCM RMS per track, again with silence removed. In simple terms, it shows how loud genres get at their loudest moments, without being skewed by brief spikes or silence. In other words,  "how hard this beat goes".

<img width="1515" height="1028" alt="2  Peak Loudness (p95) per genre" src="https://github.com/user-attachments/assets/aba35f7b-a631-4494-b19c-a97e74bb09c5" />

Once again, we see Phonk topping the chart.
Across both charts, Phonk consistently sits at the top. The data show that Phonk is not only extremely loud, but that it maintains this level of loudness across entire tracks. That sustained loudness is its defining trait. Other genres can reach peak levels close to Phonk, but they do not stay there for long.

This is especially clear when looking at more traditional electronic genres such as House, Trance, EDM, and Drum & Bass. These genres move up noticeably in the 95th percentile chart, indicating that they regularly hit very high peaks -- the "drops" -- but their average loudness remains lower because those peaks are short-lived.
On the opposite end of the spectrum, extreme metal genres such as Deathcore, Grindcore, and Technical Death Metal show a different pattern. They do not spike as high in peak loudness, but they maintain a consistently elevated loudness level across most of their duration, resulting in higher average values without extreme peaks.
Last thing worth pointing out at the other end of the chart is blues and jazz. They consistently sit at the bottom in both average and peak loudness as one would expect. These genres are built around dynamics, space, and contrast. Quiet parts are meant to be quiet, and loud moments are relative rather than absolute. 

After looking at how loud genres are on average and how loud their peaks are, it’s time to look at sustained loudness. This chart ranks genres by the ratio of median to 95th percentile RMS. In simple terms, it shows how long a genre stays close to its loudest moments. 

<img width="1693" height="872" alt="3  Sustained Loudness Per Genre" src="https://github.com/user-attachments/assets/40ff0dde-bcc0-46b5-a424-38bd4f9c7f69" />

This chart shows a stark contrast between Electronic, Extreme Metal and "classic" metal/rock music. 

We see that Extreme Metal genres (Grindcore, Tech Death, Black Metal, Deathcore, Melodeath) constantly remain near their peak loudness. These peaks are not as high as ones seen in Electronic music, but they are much more sustained. Especially compared to traditional "drop" based club music such as House and EDM. Even Phonk does not sustain its peak levels for such a long time. 

Meanwhile, more traditional genres such as Rock, Indie, and Blues show a much wider dynamic range across their duration, spending significantly less time near their loudest moments.

With genre-specific loudness out of the way, let’s take a look at loudness over time. This chart shows how loud recorded music has been on average across different decades. Each point represents a decade, and the value is the typical loudness of tracks from that period, measured using median PCM RMS after removing silence.

<img width="1307" height="709" alt="4  Loudness by year" src="https://github.com/user-attachments/assets/56c1542f-513f-4afa-bc77-3ebcb27db3de" />

The trend shown here closely matches what many musicians and audiophiles generally accept as the history of the so-called loudness war. Loudness begins to rise noticeably in the 1980s, accelerates sharply through the 1990s, and reaches its most aggressive phase in the early 2000s. After that point, the curve flattens, indicating that while loudness has continued to creep upward, the rate of increase has slowed significantly.

What this chart captures is not a change in musical style so much as a change in mastering practice. Over time, recordings have been pushed closer to their maximum usable level, reducing dynamic range and making high average loudness the norm rather than the exception.

With that context in mind, it’s time to return to Phonk.

When I first saw how loud Phonk was in the earlier charts, I was genuinely convinced I had made a mistake. The values were so extreme that my first assumption was a bug somewhere in the pipeline. 
After all, I listen to all the genres included. If Phonk was that much louder, I would have noticed right? 
To sanity-check this, I immediately ran a query for the top 20 loudest songs in my library, ranked by their mean PCM RMS. (how loud a track is on average, if you recall)

To my surprise, all of them were Phonk.

Not only that, the top 100(!!!) tracks by average loudness were all Phonk. The first non-Phonk track appears only at rank 104, which is Hoax – Jericho, a dubstep track. From there, a handful of electronic tracks start to appear sporadically in what is otherwise a sea of Phonk. It isn’t until rank 164 that we encounter the first non-electronic track: Iscariot – Verse of the Serpent, a death metal song.
At this point, it was clear that Phonk wasn’t just an outlier by genre. It was dominating the extreme tail of the entire loudness distribution.

To give a concrete sense of just how extreme these values are, here are the top five tracks and their average PCM RMS values:

- SPURIA - WITHOUT A HEAD (0.7940737363292382)
- Xteage / Dj Shuriken666 - DEVIL ENERGY (feat. DJ Shuriken666) (0.7197169943011563)
- Fexrless - SLAUGHTER (0.7190558132346953)
- $werve/Dj Shuriken666 - SOUL STEALER (feat. Dj Shuriken666) (0.6882504204306359)
- Dxrk ダーク / $werve - NORTH SIDE BOND (0.6583114677319387)

For reference, in the context of this analysis and from an audio engineering perspective:

- 0.2 - Normal.
- 0.3 - loud.
- 0.4 - Extremely loud. 
(These values refer to normalized PCM RMS, not LUFS or dBFS)

In that context, values approaching 0.7 or higher are not just loud, they’re astronomically high. These tracks are operating far beyond what is typical even for modern, heavily compressed music.

In the following chart, we can also see the top 20 tracks ranked by 95th-percentile loudness. Unsurprisingly, all of them are Phonk.
<img width="2022" height="975" alt="7  Loudest Songs" src="https://github.com/user-attachments/assets/cd529f8a-628b-48d6-98a9-2011587e30f2" />

At this point, answering whether this data is correct -- and proving it --, is crucial.

And the answer is yes. The data are correct. There is no bug. If you, the reader, are a software engineer, you’re welcome to review my code. 
But if you somehow stumbled onto this project, have read up to this point, and still want something convincing you can verify yourself, I invite you to listen to any of the tracks listed above or shown in the chart. I promise — you will immediately understand.

Finally, a screenshot of a query I executed to check just how much louder Phonk is compared to ALL other music. 

<img width="978" height="83" alt="8  Phonk Vs Everything Else" src="https://github.com/user-attachments/assets/7f499afd-e659-4068-a1db-e17f8bd240fb" />

### 2. Sonic Variance

While the primary goal of this project was studying loudness, the collected data allow for a few additional and interesting observations.

The following chart looks at sonic variety at the album level. Albums that score high here show large variation in spectral features, dynamics, and overall sound character across their runtime.
For each album, I first establish what that album typically sounds like by computing median values across several audio features including loudness, brightness, noisiness, high-frequency energy, and a simple measure of dynamic range. This median profile acts as the album’s sonic baseline.
Each track on the album is then compared against that baseline. For every feature, I measure how far the track deviates from the album’s median value. These deviations are summed and averaged across all tracks, resulting in a single score that represents how much the album’s sound shifts from track to track.

Sonic variance, as presented here, is not the same as dynamic range. Dynamic range describes variation within a track over time. Sonic variance describes variation between tracks across an album. In short, sonic variance answers a very specific question: how different do the tracks on this album sound from each other, on average, when measured using objective audio features?

<img width="1571" height="686" alt="5  Sonic Variety" src="https://github.com/user-attachments/assets/6c0d50c1-7cac-4cd1-aa0d-6a0fda799893" />

This chart presents a picture that can look chaotic; or very clear, depending on how you approach it. The highest-scoring albums come from a wide range of genres and artists, reinforcing the idea that sonic variance is not tied to style, heaviness, or experimentation alone.

Even so, a minor trend does appear. Opeth (Progressive Metal) shows up multiple times, which is unsurprising given that progressive metal often features long compositions with frequent shifts in tempo, texture, and density. It’s worth stressing that a high sonic variance score does not imply that an album is better, more creative, or more experimental. Likewise, a low score does not imply monotony. Sonic variance measures contrast, not artistic intent, cohesion, or narrative flow. An album can be sonically varied and still feel cohesive, just as an album can be sonically consistent and still be engaging.

One limitation of this metric is that a single extreme outlier track can meaningfully raise an album’s variance score, even if most tracks are relatively similar. This is exactly what happened here with The Octagonal Stairway by Pig Destroyer, a grindcore album that contains a couple of very “out-of-place” ambient tracks.

As with all aggregate statistics, the results should be interpreted as descriptive rather than definitive.

Nevertheless, having listened to all these albums, I highly recommend you do so as well. They truly are great. 

Finally, a similar process was executed at the artist level, aggregating all tracks by an artist regardless of album or release. To avoid the same kind of edge case seen above, this query was limited to artists with more than 20 tracks in my library.

<img width="1994" height="1059" alt="6  Sonic Variance per Track for each artist" src="https://github.com/user-attachments/assets/4bf42727-d86a-4281-a558-b7ae4937fc27" />

Once again, the results appear chaotic. But they become clear if you’re familiar with the artists involved.

Buckethead tops the chart. A solo musician famous for his range and sheer variety of output. Throughout the entire development of this project, if there was one thing I would have bet my life on before seeing any data, it’s that Buckethead would top this kind of chart.

Beyond that, almost every entry shown here — if not all of them — is an artist known for their range, evolution, and not only long-term influence within their respective genres, but in many cases the creation of entirely new ones through their music. 

If there’s one thing I’d want you to take away from this study, it’s this list of artists. Take some time, have a listen, and see for yourself what it is that places them in the top ten among thousands in my library.
Of course, this chart does not claim that these artists are “better” than others. It simply shows that, taken as a whole, their music spans a wider sonic space than most.


## 4. Final Thoughts

This project started because of the following simple question; I like music, I like C++, what can I do with both? 

Along the way, it confirmed what many musicians and audio engineers already believe: music is getting louder. But more importantly, it put numbers behind that belief. It showed how different genres use loudness differently, how peak loudness, average loudness, and sustained loudness describe fundamentally different behaviors, and how “variety” can be defined in measurable terms rather than vague descriptions.

It also genuinely surprised me. Phonk did not merely rank as “very loud”; it dominated the extreme end of the loudness distribution to a degree that initially looked like a bug. It wasn’t. Phonk represents a modern extreme in sustained signal density that exceeds even the loudest corners of metal and electronic music. Seeing that confirmed repeatedly, across multiple metrics and sanity checks, was one of the most striking outcomes of the entire project.

From an engineering perspective, this project served its original purpose as well. It demonstrates a complete, high-performance audio analysis pipeline built around real-world constraints: real data, real DSP, real scaling limits, and real trade-offs. Nothing here relies on synthetic datasets, black-box APIs, or hand-waved assumptions as is often the case with solo projects. It turns out that years of obsessively collecting and organizing music can become a surprisingly solid foundation for large-scale analysis.

Again, this is not formal research. It is not peer-reviewed, and it makes no claim to universality. It reflects one large, carefully curated music library and the biases that naturally come with it. But within that scope, the results are internally consistent and technically sound. 

If there is one takeaway, it’s this:many things we talk about informally in music - loudness, intensity, variety - can be measured meaningfully, as long as we are precise about what we are measuring and how. 
At the very least, this project answered my original question.

