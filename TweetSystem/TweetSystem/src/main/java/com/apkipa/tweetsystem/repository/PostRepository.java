package com.apkipa.tweetsystem.repository;

import com.apkipa.tweetsystem.model.EAudit;
import com.apkipa.tweetsystem.model.Post;
import com.apkipa.tweetsystem.model.PostTable;
import com.apkipa.tweetsystem.model.UserRelationshipTableEx;
import org.babyfish.jimmer.spring.repository.JRepository;
import org.babyfish.jimmer.spring.repository.SpringOrders;
import org.babyfish.jimmer.sql.ast.LikeMode;
import org.babyfish.jimmer.sql.ast.Predicate;
import org.babyfish.jimmer.sql.fetcher.Fetcher;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;

import java.util.List;

public interface PostRepository extends JRepository<Post, Long> {
    List<Post> findAllByUserId(Long userId);
    List<Post> findAllByUserId(Long userId, Fetcher<Post> fetcher);

    PostTable table = PostTable.$;

    default List<Post> listPassedByUserId(Long userId) {
        return sql()
                .createQuery(table)
                .where(table.auditState().eq(EAudit.Passed))
                .where(table.userId().eq(userId))
                .orderBy(table.publishTime().desc())
                .select(table)
                .execute();
    }
    default List<Post> listPassedByUserId(Long userId, Fetcher<Post> fetcher) {
        return sql()
                .createQuery(table)
                .where(table.auditState().eq(EAudit.Passed))
                .where(table.userId().eq(userId))
                .orderBy(table.publishTime().desc())
                .select(table.fetch(fetcher))
                .execute();
    }
    default List<Post> listNotRejectedByUserId(Long userId, Fetcher<Post> fetcher) {
        return sql()
                .createQuery(table)
                .where(table.auditState().ne(EAudit.Rejected))
                .where(table.userId().eq(userId))
                .orderBy(table.publishTime().desc())
                .select(table.fetch(fetcher))
                .execute();
    }
    default Page<Post> listPassedByUserIds(Pageable pageable, List<Long> userIds, Fetcher<Post> fetcher) {
        return pager(pageable).execute(
                sql()
                        .createQuery(table)
                        .where(table.auditState().eq(EAudit.Passed))
                        .where(table.userId().in(userIds))
                        .orderBy(SpringOrders.toOrders(table, pageable.getSort()))
                        .select(table.fetch(fetcher))
        );
    }
    default List<Post> listAllInProgress() {
        return sql()
                .createQuery(table)
                .where(table.auditState().eq(EAudit.InProgress))
                .orderBy(table.publishTime().desc())
                .select(table)
                .execute();
    }

    default List<Post> searchPassedByContent(String content, Long initiatedUserId, Fetcher<Post> fetcher) {
        var relation = UserRelationshipTableEx.$;
        return sql()
                .createQuery(table)
                .whereIf(
                        initiatedUserId != null,
                        sql()
                                .createSubQuery(relation)
                                .where(
                                        relation
                                                .block()
                                                .eq(true),
                                        relation
                                                .targetUser()
                                                .id()
                                                .eq(initiatedUserId)
                                )
                                .notExists()
                )
                .where(table.auditState().eq(EAudit.Passed))
                .where(table.content().like(content, LikeMode.ANYWHERE))
                .orderBy(table.publishTime().desc())
                .select(table.fetch(fetcher))
                .execute();
    }
}
