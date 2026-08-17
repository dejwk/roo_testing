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

profile_guard = rule(
    implementation = _profile_guard_impl,
    attrs = {
        "environment": attr.string(mandatory = True),
        "required_constraints": attr.label_list(
            mandatory = True,
            providers = [platform_common.ConstraintValueInfo],
        ),
    },
)
