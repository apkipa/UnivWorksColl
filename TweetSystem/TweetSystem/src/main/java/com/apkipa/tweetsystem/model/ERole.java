package com.apkipa.tweetsystem.model;

import org.babyfish.jimmer.sql.EnumType;

@EnumType(EnumType.Strategy.NAME)
public enum ERole {
    ROLE_NONE,
    ROLE_USER,
    ROLE_ADMIN,
    ROLE_REVIEW,
}
