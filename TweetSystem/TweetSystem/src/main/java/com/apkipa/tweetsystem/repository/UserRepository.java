package com.apkipa.tweetsystem.repository;

import com.apkipa.tweetsystem.model.User;
import com.apkipa.tweetsystem.model.UserFetcher;
import com.apkipa.tweetsystem.model.UserTable;
import org.babyfish.jimmer.spring.repository.JRepository;
import org.babyfish.jimmer.sql.ast.LikeMode;
import org.babyfish.jimmer.sql.fetcher.Fetcher;

import java.util.List;
import java.util.Optional;

public interface UserRepository extends JRepository<User, Long> {
    Optional<User> findByName(String name);
    Optional<User> findByName(String name, Fetcher<User> fetcher);
    boolean existsByName(String name);

    UserTable table = UserTable.$;

    default List<User> searchByNames(String name, Fetcher<User> fetcher) {
        return sql()
                .createQuery(table)
                .where(table.name().like(name, LikeMode.ANYWHERE))
                .where(table.nickname().like(name, LikeMode.ANYWHERE))
                .select(table.fetch(fetcher))
                .execute();
    }
}
