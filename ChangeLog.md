# Changelog

## Version 1.0.0 - 2021-04-07

- Optional asterisk reconnection. Fixes [#32](https://github.com/daniele77/aricpp/issues/32) 
- Fix ToString(JsonTree). Fixes [#30](https://github.com/daniele77/aricpp/issues/30) 
- Add high-level handler for DTMF received event
- Using std::chrono parameters instead of ints. Fixes [#35](https://github.com/daniele77/aricpp/issues/35) 
- Channel is not copyable nor movable. Fixes [#36](https://github.com/daniele77/aricpp/issues/36) 
- Move library header files in include/aricpp directory. Fixes [#40](https://github.com/daniele77/aricpp/issues/40) 
- Add cmake support. Fixes [#42](https://github.com/daniele77/aricpp/issues/42) 

## Version 0.9.1 - 2020-10-29

- Expose Id of the bridge

## Version 0.9.0 - 2020-06-15

- Add Recording::Mute, Recording::Unmute, Recording::Abort and Recording::Delete
- Rename Channel::Call method to Channel::Dial
- Add Channel::Continue method
- Fix Playback::Stop method

## Version 0.8.0 - 2020-04-20

- Add Recording::Pause and Recording::Resume
- Add new example for playing and recording
- Remove error message when a channel var set event is not set

## Version 0.7.0 - 2019-06-13

- Manage Global Variable events

## Version 0.6.0 - 2019-04-15

- Solve an issue with stop record function
- The bridge creation is now delegated to the AriModel class (for uniformity with Channel)
- Complete management of playbacks
- Channel variables notifications
- Monodirectional audio support when adding a channel to a bridge

## Version 0.5.0 - 2019-02-14

- Now the bridge can only be instantiated by a static factory method (to avoid a race in the callback)
- Add dtmf_events, proxy_media and video_sfu to bridge creation types

## Version 0.4.1 - 2018-04-20

- Fix bug in Channel::GetVar

## Version 0.4.0 - 2018-04-20

- Add creation only of a asterisk channel (w/o dial)
- Add dial of a asterisk channel
- Add Channel State ToString function
- Use url parameter instead of http body for channel Get variable

## Version 0.3.0 - 2018-02-28

- URL encode in ARI http messages

## Version 0.2.0 - 2018-01-26

- Use boost library v. 1.66.0 or later (does not need separate beast library anymore)
- Add GetVar in Channel
- Hangup on Channel destructor
- Add holding Bridge type
- Add holding_bridge sample

## Version 0.1.0 - 2017-10-20

- Use beast library v. 123
- Add CONTRIBUTING and CODE_OF_CONDUCT files
- Add channel destruction Q.850 cause
- Remove std::move when prevents RVO

## Version 0.0.3 - 2017-08-28 

- Use boost version of beast library
- Add Channel features
- Add Bridge features
- Fix linking problem whith multiple inclusion of channel.h
- High level interface for recordings
- Various bug fix

## Version 0.0.2 - 2017-03-02

- Basic high level interface (Channel)
- New samples
- Various bug fix

## Version 0.0.1 - 2017-02-02

- Initial export

