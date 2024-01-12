package com.apkipa.tweetsystem.model;

import org.babyfish.jimmer.sql.EnumType;

@EnumType(EnumType.Strategy.NAME)
public enum EAudit {
    Draft,
    InProgress,
    Passed,
    Rejected,
}
