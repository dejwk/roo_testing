"""Hard analysis guards for roo_testing build-profile constraints."""

def _profile_guard_impl(ctx):
    missing = []
    for target in ctx.attr.required_constraints:
        constraint = target[platform_common.ConstraintValueInfo]
        if not ctx.target_platform_has_constraint(constraint):
            missing.append(str(target.label))

    if missing:
        fail(
            "%s requires target-platform constraint(s) %s; " % (
                ctx.attr.environment,
                ", ".join(missing),
            ) +
            "select a matching roo_testing frontend profile in the root workspace",
        )

    return [DefaultInfo()]

_profile_guard = rule(
    implementation = _profile_guard_impl,
    attrs = {
        "environment": attr.string(mandatory = True),
        "required_constraints": attr.label_list(
            mandatory = True,
            providers = [platform_common.ConstraintValueInfo],
        ),
    },
)

def profile_guard(name, environment, required_constraints):
    """Reject explicit profile mismatches while allowing wildcards to skip them.

    Compatibility propagates through callers, so an ESP32 wildcard does not
    analyze Pico libraries (or other frontend/SoC contracts). Explicit requests
    still fail with Bazel's incompatible-target diagnostic.

    Args:
        name: Target name for the profile boundary.
        environment: Human-readable environment used in guard diagnostics.
        required_constraints: Platform constraints inherited by all dependents.
    """
    _profile_guard(
        name = name,
        environment = environment,
        required_constraints = required_constraints,
        target_compatible_with = required_constraints,
    )
