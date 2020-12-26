# Sentry.io Cash Reporting

FlightGear can optionally report crashes and serious errors to Sentry.io. We have a sponsored
account provided gratis by Sentry; for access to this, ask on the developer list.

## Error conditions

If FlightGear crahses, Sentry will automatically submit a report. For non-crash errors,
we manually submit an exception report. At present this is done whenever we trigger the
`fatalMessageBox`, and in other serious situations. Deciding where is appropriate (or not)
to report the error to Sentry is a key challenge of the system, since we don't want to
report user misconfiguration problems, but we do want to detect recurring and
systematic failures, eg broken aircraft.

In general since we have (almost) unlimited event limits on our sponsored plan, it's 
better to send errors and filter them on the server side, but this needs to be done 
with some intelligence; for example we do not (at present) report Nasal runtime
errors, since this might overload the system.

## Data Protection

We explicitly do not include any personal information in the reports, and disable IP address
collection. This avoids any GPDR obligations for us.
Since this makes it hard to cluster reports by user, we instead generate a UUID
corresponding to a FlightGear installation. This gives us a way to anonymously cluster reports
by computer, without any personal data disclosure. (So we can determine if a hundred reports of a
crash come from fifty discrete users, or just one)

## Supplementatal Data

We record various pieces of configuration state as 'tags': such as the OS, graphics card,
major settings (eg, is multi-player enabled, which aircraft is being flown) as _tags_ in
Sentry terminology. This allows grouping of reports by similar tags; for example we can
identify that a particular crash only occurs with Intel Graphics, or when flying the
C172.

Additionally we record 'breadcrumbs'; these are included with an error report if one
is sent, and give an idea of the path of the user through the application, prior to the
crash. We have breadcrumbs for key events such as the splash-screen completing, the
user changing position, or scenery being reloaded.

`WARN` and `ALERT` level `SG_LOG` messages are currently included as breadcrumbs automatically;
this means it's important not to casually add messages at the levels for non-serious conditions.
The integration code has a list of commonly ocurring but non-useful messages which are
skipped from sending; especially some OSG ones related to PNG and AC3D data issues.

Adding new tags or breadcrumbs should be done with care, but is generally useful, and
suggestions in this area are appreciated.

## API

All the API is contained in `sentryIntegration.hxx`, inside the `flightgear` namespace. 
The methods are no-ops if the Sentry SDK was not available at CMake time, and are also
no-ops if the user has disabled sentry reporting.

## Building

The API requires the injection of a private API key to report to our account; this
must be kept private of course, so it's injected into official builds at CMake time
on Jenkins, via the environment variable `FLIGHTGEAR_SENTRY_API_KEY`. 

The build must be configured to produce debug symbols, which are uploaded to Sentry
via the `sentry-cli` tool as part of our build scripts.