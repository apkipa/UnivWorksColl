package com.apkipa.tweetsystem.misc;

import org.babyfish.jimmer.sql.runtime.ScalarProvider;
import org.springframework.stereotype.Component;

import java.time.Instant;

@Component
public class InstantScalarProvider extends ScalarProvider<Instant, String> {

    @Override
    public Instant toScalar(String sqlValue) {
        return Instant.parse(sqlValue);
    }

    @Override
    public String toSql(Instant scalarValue) {
        return scalarValue.toString();
    }
}
